#include "io/v4l2_capture.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/media.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <unistd.h>

// The REAL struct definition (Hidden from the user)
typedef struct {
  void *start;
  size_t length;
} v4l2_mmap_buffer_t;

struct v4l2_device_t {
  int fd;
  uint32_t width;
  uint32_t height;
  uint32_t pixel_format;
  size_t stride;
  v4l2_mmap_buffer_t buffers[V4L2_REQ_BUFFER_COUNT];
  uint32_t buffer_count;
  bool is_streaming;
};

/*
 * Discovers the correct media node, configures the MIPI pipeline,
 * and returns the correct /dev/videoX path (e.g. "/dev/video0").
 * The caller must free() the returned string.
 */
char *v4l2_auto_setup_pipeline(uint32_t width, uint32_t height) {
  char media_path[32] = {0};
  int found = 0;

  // 1. Find the correct /dev/mediaX node for the Pi 5 (rp1-cfe)
  for (int i = 0; i < 32; i++) {
    snprintf(media_path, sizeof(media_path), "/dev/media%d", i);
    int fd = open(media_path, O_RDWR);
    if (fd >= 0) {
      struct media_device_info info;
      if (ioctl(fd, MEDIA_IOC_DEVICE_INFO, &info) == 0) {
        // Check if the driver is the Raspberry Pi 5 camera receiver
        if (strncmp(info.driver, "rp1-cfe", 7) == 0) {
          found = 1;
          close(fd);
          break;
        }
      }
      close(fd);
    }
  }

  if (!found) {
    fprintf(stderr, "Error: Could not find rp1-cfe media node. Is the camera "
                    "plugged in?\n");
    return NULL;
  }

  printf("Discovered Pi 5 Camera Pipeline at: %s\n", media_path);

  // 2. Execute the media-ctl configuration dynamically
  char cmd[512];

  // Reset topology
  snprintf(cmd, sizeof(cmd), "media-ctl -d %s -r", media_path);
  system(cmd);

  // Link CSI2 to the Capture Channel
  snprintf(cmd, sizeof(cmd),
           "media-ctl -d %s -l '\"csi2\":4 -> \"rp1-cfe-csi2_ch0\":0[1]'",
           media_path);
  system(cmd);

  // Format the IMX219 Sensor Pad
  snprintf(cmd, sizeof(cmd),
           "media-ctl -d %s -V '\"imx219 11-0010\":0 [fmt:SRGGB10_1X10/%dx%d "
           "field:none colorspace:raw]'",
           media_path, width, height);
  system(cmd);

  // Format the CSI2 Receiver Pads
  snprintf(cmd, sizeof(cmd),
           "media-ctl -d %s -V '\"csi2\":0 [fmt:SRGGB10_1X10/%dx%d field:none "
           "colorspace:raw]'",
           media_path, width, height);
  system(cmd);

  snprintf(cmd, sizeof(cmd),
           "media-ctl -d %s -V '\"csi2\":4 [fmt:SRGGB10_1X10/%dx%d field:none "
           "colorspace:raw]'",
           media_path, width, height);
  system(cmd);

  printf("Media Controller pipeline configured for %dx%d RAW10.\n", width,
         height);

  // 3. Ask media-ctl for the exact /dev/videoX node this pipeline is using
  snprintf(cmd, sizeof(cmd), "media-ctl -d %s -e \"rp1-cfe-csi2_ch0\"",
           media_path);
  FILE *fp = popen(cmd, "r");
  if (!fp)
    return NULL;

  char *video_node = malloc(32);
  if (fgets(video_node, 32, fp) != NULL) {
    // Remove the trailing newline character from the output
    video_node[strcspn(video_node, "\n")] = 0;
  }
  pclose(fp);

  return video_node; // e.g., returns "/dev/video0" dynamically!
}

/* Allocates memory, opens the device, and applies the config */
v4l2_device_t *v4l2_device_open(const v4l2_config_t *config) {
  if (!config || !config->device_path)
    return NULL;

  // Allocate memory for our state
  struct v4l2_device_t *dev = malloc(sizeof(struct v4l2_device_t));
  if (!dev)
    return NULL;

  // Open video device
  dev->fd = open(config->device_path, O_RDWR | O_NONBLOCK);
  if (dev->fd < 0) {
    perror("Unable to open video device");
    free(dev);
    return NULL; // Return NULL on pointer failure, not -1
  }

  // Query Capabilities
  struct v4l2_capability cap; // Fixed typo: capability
  if (ioctl(dev->fd, VIDIOC_QUERYCAP, &cap) < 0) {
    perror("VIDIOC_QUERYCAP failed");
    close(dev->fd);
    free(dev);
    return NULL;
  }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
      !(cap.capabilities & V4L2_CAP_STREAMING)) {
    printf("Device does not support capture or streaming\n");
    close(dev->fd);
    free(dev);
    return NULL;
  }

  // Set format using the user's config
  struct v4l2_format fmt = {0};
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = config->width;
  fmt.fmt.pix.height = config->height;
  fmt.fmt.pix.pixelformat = config->pixel_format;
  fmt.fmt.pix.field = V4L2_FIELD_NONE;

  if (ioctl(dev->fd, VIDIOC_S_FMT, &fmt) < 0) {
    perror("VIDIOC_S_FMT failed");
    close(dev->fd);
    free(dev);
    return NULL;
  }

  // Store the actual negotiated hardware values in our private struct
  dev->width = fmt.fmt.pix.width;
  dev->height = fmt.fmt.pix.height;
  dev->pixel_format = fmt.fmt.pix.pixelformat;
  dev->stride = fmt.fmt.pix.bytesperline;
  dev->is_streaming = false;
  dev->buffer_count =
      V4L2_REQ_BUFFER_COUNT; // Ensure this matches the header macro

  return dev; // Hand back the opaque pointer!
}

/* Queues buffers and turns the stream on */
int v4l2_device_start(v4l2_device_t *dev) {
  if (!dev)
    return -1;

  struct v4l2_requestbuffers req = {0};
  req.count = dev->buffer_count;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (ioctl(dev->fd, VIDIOC_REQBUFS, &req) < 0) {
    perror("VIDIOC_REQBUFS failed");
    return -1;
  }

  // Map Buffers and queue them immediately
  for (int i = 0; i < req.count; i++) {
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;

    if (ioctl(dev->fd, VIDIOC_QUERYBUF, &buf) < 0) {
      perror("VIDIOC_QUERYBUF failed");
      return -1;
    }

    dev->buffers[i].length = buf.length;
    dev->buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, dev->fd, buf.m.offset);

    if (dev->buffers[i].start == MAP_FAILED) {
      perror("mmap failed");
      return -1;
    }

    // Push the empty buffer to the hardware queue
    if (ioctl(dev->fd, VIDIOC_QBUF, &buf) < 0) {
      perror("VIDIOC_QBUF failed");
      return -1;
    }
  }

  // Turn on the stream
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl(dev->fd, VIDIOC_STREAMON, &type) < 0) {
    perror("VIDIOC_STREAMON failed");
    return -1;
  }

  dev->is_streaming = true;
  return 0;
}

/* Blocks until a SINGLE frame is ready, then populates the frame struct */
int v4l2_device_dqbuf(v4l2_device_t *dev, v4l2_frame_t *frame,
                      int timeout_sec) {
  if (!dev || !frame)
    return -1;

  fd_set fds;
  struct timeval tv;
  FD_ZERO(&fds);
  FD_SET(dev->fd, &fds);
  tv.tv_sec = timeout_sec;
  tv.tv_usec = 0;

  int r = select(dev->fd + 1, &fds, NULL, NULL, &tv);

  if (r == -1) {
    perror("select error");
    return -1;
  } else if (r == 0) {
    printf("Timeout waiting for frame!\n");
    return -1;
  }

  struct v4l2_buffer dqbuf = {0};
  dqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  dqbuf.memory = V4L2_MEMORY_MMAP;

  if (ioctl(dev->fd, VIDIOC_DQBUF, &dqbuf) < 0) {
    perror("VIDIOC_DQBUF failed");
    return -1;
  }

  // Expose the data to the user!
  frame->data = dev->buffers[dqbuf.index].start;
  frame->size = dqbuf.bytesused;
  frame->index = dqbuf.index;

  return 0;
}

/* Returns the buffer back to the hardware pipeline */
int v4l2_device_qbuf(v4l2_device_t *dev, uint32_t buffer_index) {
  if (!dev)
    return -1;

  struct v4l2_buffer qbuf = {0};
  qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  qbuf.memory = V4L2_MEMORY_MMAP;
  qbuf.index = buffer_index; // Pass the ticket back!

  if (ioctl(dev->fd, VIDIOC_QBUF, &qbuf) < 0) {
    perror("VIDIOC_QBUF failed");
    return -1;
  }
  return 0;
}

/* Turns the stream off */
int v4l2_device_stop(v4l2_device_t *dev) {
  if (!dev || !dev->is_streaming)
    return 0;

  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl(dev->fd, VIDIOC_STREAMOFF, &type) < 0) {
    perror("VIDIOC_STREAMOFF failed");
    return -1;
  }

  dev->is_streaming = false;
  return 0;
}

/* Closes the device and frees memory */
void v4l2_device_close(v4l2_device_t *dev) {
  if (!dev)
    return;

  v4l2_device_stop(dev);

  for (int i = 0; i < dev->buffer_count; ++i) {
    if (dev->buffers[i].start && dev->buffers[i].start != MAP_FAILED) {
      munmap(dev->buffers[i].start, dev->buffers[i].length);
    }
  }

  if (dev->fd >= 0) {
    close(dev->fd);
  }

  free(dev); // Free the malloc'd struct!
}