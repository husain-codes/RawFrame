#include "io/v4l2_capture.h"
#include <img/isp_pipeline.h>
#include <io/img_io.h>
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
  free(video_node);
  if (!camera) {
    fprintf(stderr, "Fatal: Could not initialize camera.\n");
    return EXIT_FAILURE;
  }

  v4l2_capture_format_t capture_format;
  if (v4l2_device_get_format(camera, &capture_format) < 0) {
    fprintf(stderr, "Fatal: Could not retrieve camera format.\n");
    v4l2_device_close(camera);
    return EXIT_FAILURE;
  }

  // Starting camera state machine
  isp_context_t *isp_ctx = isp_create();
  if (!isp_ctx) {
    fprintf(stderr, "Fatal: Could not create ISP context.\n");
    v4l2_device_close(camera);
    return EXIT_FAILURE;
  }
  isp_config_t isp_config = {.width = capture_format.width,
                             .height = capture_format.height,
                             .stride = capture_format.stride,
                             .input_format = IMG_FMT_BAYER_10BIT,
                             .output_format = IMG_FMT_RGB24,
                             .black_level = 16,
                             .enable_bls = true,
                             .enable_awb = true,
                             .enable_demosaic = true};
  if (isp_init(isp_ctx, &isp_config) < 0) {
    fprintf(stderr, "Fatal: Could not initialize ISP.\n");
    isp_destroy(isp_ctx);
    v4l2_device_close(camera);
    return EXIT_FAILURE;
  }

  // 3. Start the hardware stream
  if (v4l2_device_start(camera) < 0) {
    fprintf(stderr, "Fatal: Could not start hardware stream.\n");
    v4l2_device_close(camera);
    return EXIT_FAILURE;
  }

  if (isp_start(isp_ctx) < 0) {
    fprintf(stderr, "Fatal: Could not start ISP.\n");
    isp_destroy(isp_ctx);
    v4l2_device_close(camera);
    return EXIT_FAILURE;
  }

  // 4. The Capture Loop
  v4l2_frame_t frame = {0};
  int total_frames = 150;

  // Allocate buffers for ISP output and RGB output
  uint16_t *isp_output_buffer =
      malloc(isp_config.width * isp_config.height * sizeof(uint16_t));
  if (!isp_output_buffer) {
    fprintf(stderr, "Fatal: Could not allocate ISP output buffer.\n");
    return EXIT_FAILURE;
  }
  img_t *out_rgb =
      img_create(isp_config.width, isp_config.height, IMG_FMT_RGB24);
  if (!out_rgb) {
    fprintf(stderr, "Fatal: Could not create output RGB image.\n");
    free(isp_output_buffer);
    return EXIT_FAILURE;
  }
  for (int i = 0; i < total_frames; i++) {
    // Wait up to 2 seconds for a frame
    if (v4l2_device_dqbuf(camera, &frame, 2) == 0) {

      // Process the frame through the ISP pipeline
      if (isp_process_frame(isp_ctx, frame.data, isp_output_buffer, out_rgb,
                            i) < 0) {
        fprintf(stderr, "Error: ISP processing failed for frame %d\n", i);
        free(isp_output_buffer);
        img_destroy(out_rgb);
        break;
      }
      // Save the raw pixels to disk
      char filename[64];
      snprintf(filename, sizeof(filename), "frame_%03d.bmp", i);

      if (img_save_bmp(filename, out_rgb) < 0) {
        fprintf(stderr, "Error: Failed to save BMP for frame %d\n", i);
      } else {
        printf("Captured and saved %s\n", filename);
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
  isp_stop(isp_ctx);
  isp_destroy(isp_ctx);
  img_destroy(out_rgb);
  free(isp_output_buffer);
  printf("Shutting down camera hardware...\n");
  v4l2_device_close(camera); // This calls stop() and unmaps memory internally

  printf("Done! Successfully captured %d frames.\n", total_frames);
  return EXIT_SUCCESS;
}