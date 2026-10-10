#ifndef V4L2_CAPTURE_H
#define V4L2_CAPTURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define V4L2_REQ_BUFFER_COUNT 10

/* Forward declaration: The struct variables are completely hidden from the user
 */
typedef struct v4l2_device_t v4l2_device_t;

typedef struct {
  const char *device_path; /* e.g., "/dev/video0", or NULL for auto-discovery */
  uint32_t width;
  uint32_t height;
  uint32_t pixel_format;
  uint32_t buffer_count;
} v4l2_config_t;

typedef struct {
  void *data;
  size_t size;
  uint32_t index; /* Buffer ticket index for re-queuing */
} v4l2_frame_t;

char *v4l2_auto_setup_pipeline(uint32_t width, uint32_t height);
/*
 * Lifecycle APIs
 */
/* Allocates memory, opens the device, and applies the config */
v4l2_device_t *v4l2_device_open(const v4l2_config_t *config);

/* Queues buffers and turns the stream on */
int v4l2_device_start(v4l2_device_t *dev);

/* Turns the stream off */
int v4l2_device_stop(v4l2_device_t *dev);

/* Closes the device and frees the v4l2_device_t memory */
void v4l2_device_close(v4l2_device_t *dev);

/*
 * Capture APIs
 */
/* Blocks until a frame is ready, then populates the frame struct */
int v4l2_device_dqbuf(v4l2_device_t *dev, v4l2_frame_t *frame, int timeout_sec);

/* Returns the buffer back to the hardware pipeline */
int v4l2_device_qbuf(v4l2_device_t *dev, uint32_t buffer_index);

#endif // V4L2_CAPTURE_H