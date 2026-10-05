#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

struct v4l2_capability cap;
struct v4l2_format fmt = {0};
struct v4l2_requestbuffers req = {0};
struct Buffer {
  void *start;
  size_t length;
};
struct v4l2_buffer buf = {0};
struct timespec start_time = {0};
struct timespec current_time = {0};

int main() {
  // v4L2 Flow
  // 1) open video devices
  int fd = open("/dev/video0", O_RDWR | O_NONBLOCK);
  if (fd < 0) {
    perror("Unable to open video device\n");
    return -1;
  }

  // 2) Query Capabilities
  int ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
  if (ret < 0) {
    perror("VIDIOC_QUERYCAP failed");
    return -1;
  }

  printf("driver name: %s\n card name: %s\n", cap.driver, cap.card);

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    printf("device does not support capture\n");
    return -1;
  }

  if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
    printf("device does not support streaming\n");
    return -1;
  }

  // 3) Set Format
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = 1640;
  fmt.fmt.pix.height = 1232;
  fmt.fmt.pix.pixelformat = v4l2_fourcc('p', 'R', 'A', 'A');
  fmt.fmt.pix.field = V4L2_FIELD_NONE;

  ret = ioctl(fd, VIDIOC_S_FMT, &fmt);

  if (ret < 0) {
    perror("VIDIOC_S_FMT failed");
    return -1;
  }

  printf("Formet set:  %d x %d (Stride: %d bytes)\n", fmt.fmt.pix.width,
         fmt.fmt.pix.height, fmt.fmt.pix.bytesperline);

  // 4) Request Buffers
  req.count = 10;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  ret = ioctl(fd, VIDIOC_REQBUFS, &req);
  if (ret < 0) {
    perror("VIDIOC_REQBUFS failed");
    return -1;
  }
  printf("Allocated %d buffers\n", req.count);
  // 5) Map Buffers
  struct Buffer *buffers = calloc(req.count, sizeof(*buffers));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  for (int i = 0; i < req.count; i++) {
    buf.index = i;
    ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
    if (ret < 0) {
      perror("VIDIOC_QUERYBUF failed");
      return -1;
    }
    buffers[i].length = buf.length;
    // 6) Map the buffer into user space
    buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                            MAP_SHARED, fd, buf.m.offset);
    if (buffers[i].start == MAP_FAILED) {
      perror("");
      return -1;
    }
    printf("Buffer %d mapped at %p (length: %zu)\n", i, buffers[i].start,
           buffers[i].length);
  }

  // 7. Queue Buffers (using a FRESH struct)
  for (int i = 0; i < req.count; i++) {
    struct v4l2_buffer qbuf = {0}; // Clean struct for every loop!
    qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    qbuf.memory = V4L2_MEMORY_MMAP;
    qbuf.index = i;

    ret = ioctl(fd, VIDIOC_QBUF, &qbuf);
    if (ret < 0) {
      perror("VIDIOC_QBUF failed");
      return -1;
    }
  }
  printf("All 10 buffers queued to hardware.\n");

  // 8. Stream On (using a standard int)
  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  ret = ioctl(fd, VIDIOC_STREAMON, &type);
  if (ret < 0) {
    perror("VIDIOC_STREAMON failed");
    return -1;
  }
  printf("Stream ON! Sensor is capturing...\n");
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  printf("Start time: %ld.%09ld seconds\n", start_time.tv_sec,
         start_time.tv_nsec);
  int frame_count = 1;
  while (((current_time.tv_sec - start_time.tv_sec) * 1000) +
             ((current_time.tv_nsec - start_time.tv_nsec) / 1000000) <
         500) {
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    printf("Current time: %ld.%09ld seconds\n", current_time.tv_sec,
           current_time.tv_nsec);
    // 9. --- WAIT FOR THE FRAME ---
    fd_set fds;
    struct timeval tv;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec = 2; // Wait up to 2 seconds
    tv.tv_usec = 0;

    printf("Waiting for frame...\n");
    int r = select(fd + 1, &fds, NULL, NULL, &tv);

    if (r == -1) {
      perror("select error");
      return -1;
    } else if (r == 0) {
      printf("Timeout waiting for frame!\n");
      return -1;
    }

    // 10. --- DEQUEUE THE FILLED BUFFER ---
    struct v4l2_buffer dqbuf = {0};
    dqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    dqbuf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fd, VIDIOC_DQBUF, &dqbuf);
    if (ret < 0) {
      perror("VIDIOC_DQBUF failed");
      return -1;
    }
    printf("Dequeued frame from buffer %d! Bytes used: %d\n", dqbuf.index,
           dqbuf.bytesused);

    // 11. --- SAVE TO DISK ---
    char filename[32];
    sprintf(filename, "frame_%03d.raw", frame_count);
    FILE *fout = fopen(filename, "wb");
    if (fout) {
      fwrite(buffers[dqbuf.index].start, dqbuf.bytesused, 1, fout);
      fclose(fout);
      printf("Saved RAW data to %s\n", filename);
    } else {
      perror("Failed to open file for writing");
    }
    frame_count++;

    // 12. --- REQUEUE THE BUFFER ---
    ret = ioctl(fd, VIDIOC_QBUF, &dqbuf);
  }

  // 12. --- CLEANUP ---
  ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
  if (ret < 0) {
    perror("VIDIOC_STREAMOFF type");
    return -1;
  }
  for (int i = 0; i < req.count; ++i) {
    munmap(buffers[i].start, buffers[i].length);
  }
  free(buffers);
  close(fd);
  return 0;
}
