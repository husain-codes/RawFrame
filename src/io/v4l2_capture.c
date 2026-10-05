#include "io/v4l2_capture.h"
#include <errno.h>
#include <fcntl.h>
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