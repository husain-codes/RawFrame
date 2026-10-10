#include "io/v4l2_capture.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// A helper macro so we don't have to include <linux/videodev2.h> just for
// v4l2_fourcc
#define MAKE_FOURCC(a, b, c, d)                                                \
  ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) |              \
   ((uint32_t)(d) << 24))

int main() {
  printf("Starting RawFrame Capture...\n");

  // 1. Auto-configure the hardware pipeline and get the exact video node
  uint32_t req_width = 1640;
  uint32_t req_height = 1232;

  char *video_node = v4l2_auto_setup_pipeline(req_width, req_height);
  if (!video_node) {
    return EXIT_FAILURE;
  }
  printf("Connecting to dynamically discovered video node: %s\n", video_node);

  // 1. Fill out the order ticket (Configuration)
  v4l2_config_t config = {.device_path = video_node,
                          .width = 1640,
                          .height = 1232,
                          .pixel_format = MAKE_FOURCC('p', 'R', 'A', 'A'),
                          .buffer_count = 10};

  // 2. Open the camera
  v4l2_device_t *camera = v4l2_device_open(&config);
  if (!camera) {
    fprintf(stderr, "Fatal: Could not initialize camera.\n");
    return EXIT_FAILURE;
  }

  // 3. Start the hardware stream
  if (v4l2_device_start(camera) < 0) {
    fprintf(stderr, "Fatal: Could not start hardware stream.\n");
    v4l2_device_close(camera);
    return EXIT_FAILURE;
  }

  // 4. The Capture Loop
  v4l2_frame_t frame = {0};
  int total_frames = 150;

  for (int i = 0; i < total_frames; i++) {
    // Wait up to 2 seconds for a frame
    if (v4l2_device_dqbuf(camera, &frame, 2) == 0) {

      // Save the raw pixels to disk
      char filename[64];
      snprintf(filename, sizeof(filename), "frame_%03d.raw", i);

      FILE *fout = fopen(filename, "wb");
      if (fout) {
        fwrite(frame.data, 1, frame.size, fout);
        fclose(fout);
        printf("Captured and saved %s (Size: %zu bytes)\n", filename,
               frame.size);
      }

      // IMMEDIATELY hand the buffer ticket back to the hardware
      if (v4l2_device_qbuf(camera, frame.index) < 0) {
        fprintf(stderr, "Warning: Failed to requeue buffer %d\n", frame.index);
      }

    } else {
      fprintf(stderr, "Error: Timeout or failure reading frame %d\n", i);
      break; // Exit loop on failure
    }
  }

  // 5. Clean Teardown
  printf("Shutting down camera hardware...\n");
  v4l2_device_close(camera); // This calls stop() and unmaps memory internally

  printf("Done! Successfully captured %d frames.\n", total_frames);
  return EXIT_SUCCESS;
}