#include <img/img.h>
#include <img/img_format.h>
#include <img/img_processing.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations for the internal blur helper functions. These are not
 * exposed in the header since they are implementation details of the box blur
 * algorithm. They operate on raw pixel buffers and are called by img_box_blur
 * for each plane and channel as needed. The horizontal blur computes
 * intermediate sums in a 16-bit buffer to avoid overflow, while the vertical
 * blur applies the final averaging and writes back to the output image. Both
 * functions take into account the pixel stride to correctly handle interleaved
 * formats like RGB24.
 */
static int blur_horizontal(const uint8_t *src, uint16_t *dst, int width,
                           int height, int src_stride, int dst_stride,
                           int radius, int pixel_stride);
/* The vertical blur function takes the intermediate 16-bit sums from the
 * horizontal pass and applies the vertical component of the box blur. It
 * divides the accumulated sum by the kernel area to compute the average And
 * writes the final blurred pixel values back to the output image. The
 * pixel_stride parameter ensures that the function correctly handles both
 * planar and interleaved formats by applying the blur to the appropriate
 * channels. The function processes each column of pixels independently, sliding
 * the vertical window down the image And updating the window sum efficiently by
 * adding the new pixel entering the window and subtracting the pixel leaving
 * the window, similar to the horizontal pass.
 */
static int blur_vertical(const uint16_t *src, uint8_t *dst, int width,
                         int height, int src_stride, int dst_stride, int radius,
                         int pixel_stride);
/*
 * Applies a box blur filter to the input image and stores the result in the
 * output image. The function checks for valid input parameters, including
 * matching dimensions and formats between the source and destination images,
 * as well as a valid kernel size. It currently supports only non-planar
 * formats (e.g., RGB24, RGBA32) by applying the blur to the interleaved pixel
 * data. For planar formats, a TODO is noted to handle them separately by
 * processing each plane independently. returns 1 on success, -1 on failure.
 */
int img_box_blur(const img_t *src, img_t *dst, int kernel_size) {
  if (!src || !dst) {
    fprintf(stderr, "Src or dst image is null\n");
    return -1;
  }

  if (src->width != dst->width || src->height != dst->height ||
      src->format != dst->format) {
    fprintf(stderr,
            "Source and destination images must match dimensions and format\n");
    return -1;
  }

  if (kernel_size < 3 || kernel_size % 2 == 0) {
    fprintf(stderr, "Invalid kernel size: must be odd and >= 3\n");
    return -1;
  }

  int radius = kernel_size / 2;

  // Get the total number of channels / pixel_stride for this format
  // For RGB24, this returns 3. For GRAY8 / YUV planes, this returns 1.
  int channels = img_format_bytes_per_pixel(src->format);

  // Loop through every physical allocation plane
  for (int pi = 0; pi < src->num_planes; pi++) {

    // For planar formats, each plane is 1 channel. For packed RGB, it's
    // 'channels'.
    int plane_pixel_stride = (src->num_planes > 1) ? 1 : channels;

    // Handle YUV 4:2:0 sub-sampling adjustments for chroma planes (pi > 0)
    int plane_width = (pi > 0 && (src->format == IMG_FMT_NV12 ||
                                  src->format == IMG_FMT_YUV420P))
                          ? src->width / 2
                          : src->width;
    int plane_height = (pi > 0 && (src->format == IMG_FMT_NV12 ||
                                   src->format == IMG_FMT_YUV420P))
                           ? src->height / 2
                           : src->height;

    // Guard against images smaller than the blur kernel
    if (plane_width < kernel_size || plane_height < kernel_size) {
      // Copy the plane untouched so it doesn't turn black
      size_t plane_bytes = (size_t)src->stride[pi] * plane_height;
      memcpy(dst->planes[pi], src->planes[pi], plane_bytes);
      printf(
          "Plane %d is too small (%dx%d) for kernel %d. Copying untouched.\n",
          pi, plane_width, plane_height, kernel_size);
      continue; // Skip this plane but continue processing others if valid
    }

    // For NV12 chroma plane (pi == 1), the UV channels are interleaved together
    if (pi == 1 && src->format == IMG_FMT_NV12) {
      plane_pixel_stride = 2;
    }

    // Strategy 1: Calculate intermediate 16-bit stride in terms of elements
    int tmp_stride_elements = plane_width * plane_pixel_stride;
    size_t tmp_size_bytes =
        (size_t)tmp_stride_elements * plane_height * sizeof(uint16_t);

    uint16_t *tmp_buf = (uint16_t *)malloc(tmp_size_bytes);
    if (!tmp_buf) {
      fprintf(stderr, "Failed to allocate intermediate 16-bit buffer\n");
      return -1;
    }

    // --- THE INTERLEAVED CHANNEL LANE ROUTER ---
    // Loops 'plane_pixel_stride' times (3 for RGB, 1 for Mono Y, 2 for NV12 UV)
    for (int ch = 0; ch < plane_pixel_stride; ch++) {

      // Pass the base pointers shifted forward by 'ch' bytes/elements
      blur_horizontal(src->planes[pi] + ch, tmp_buf + ch, plane_width,
                      plane_height, src->stride[pi], tmp_stride_elements,
                      radius, plane_pixel_stride);

      blur_vertical(tmp_buf + ch, dst->planes[pi] + ch, plane_width,
                    plane_height, tmp_stride_elements, dst->stride[pi], radius,
                    plane_pixel_stride);
    }
    free(tmp_buf);
  }

  printf("Blurred source image successfully\n");
  return 0;
}

static int blur_horizontal(const uint8_t *src, uint16_t *dst, int width,
                           int height, int src_stride, int dst_stride,
                           int radius, int pixel_stride) {
  uint16_t window_sum = 0;
  int kernel_size = 2 * radius + 1;
  // Guard against images narrower than the blur kernel
  if (width < kernel_size) {
    return -1;
  }
  int src_pixel_offset;
  int dst_pixel_offset;
  int src_row_offset;
  int dst_row_offset;

  for (int y = 0; y < height; y++) {
    window_sum = 0;
    dst_row_offset = y * dst_stride;
    src_row_offset = y * src_stride;

    // Phase 1: Pre-load the first window chunk (Columns 0 to kernel_size - 1)
    for (int i = 0; i < kernel_size; i++) {
      window_sum += src[src_row_offset + i * pixel_stride];
    }
    // --- CRITICAL DETAIL FOR PATH A ---
    // The pixels from x = 0 up to x = kernel_size - 1 need an initial value
    // written to dst! Since the window hasn't started sliding yet, you can
    // assign this initial 'window_sum' to those skipped starting columns so
    // they aren't left as garbage memory.
    for (int i = 0; i < kernel_size; i++) {
      dst[dst_row_offset + i * pixel_stride] = window_sum;
    }

    // Phase 2: Start sliding strictly *after* the initialized chunk
    for (int x = kernel_size; x < width; x++) {
      dst_pixel_offset = dst_row_offset + x * pixel_stride;
      src_pixel_offset =
          src_row_offset + x * pixel_stride; // why since src in 8 bit

      int leave_offset = src_row_offset + (x - kernel_size) * pixel_stride;
      window_sum = window_sum + src[src_pixel_offset] - src[leave_offset];
      dst[dst_pixel_offset] = window_sum;
    }
  }
  return 0;
}

static int blur_vertical(const uint16_t *src, uint8_t *dst, int width,
                         int height, int src_stride, int dst_stride, int radius,
                         int pixel_stride) {
  int kernel_size = 2 * radius + 1;
  int kernel_area = kernel_size * kernel_size;
  uint32_t window_sum = 0;
  int src_col_offset;
  int dst_col_offset;
  int src_pixel_offset;
  int dst_pixel_offset;
  for (int x = 0; x < width; x++) {
    window_sum = 0;
    src_col_offset = x * pixel_stride; // since src is in 16 bit, the stride is
                                       // in terms of uint16_t elements
    dst_col_offset = x * pixel_stride; // since dst is in 8 bit, the stride is
                                       // in terms of uint8_t elements

    // Guard against images shorter than the blur kernel
    if (height < kernel_size) {
      return -1;
    }

    // Calculate initial window sum
    for (int i = 0; i < kernel_size; i++) {
      window_sum += src[src_col_offset + i * src_stride];
    }

    // Allocate initial window sum to initial pixels
    for (int i = 0; i < kernel_size; i++) {
      dst[dst_col_offset + i * dst_stride] =
          (uint8_t)((window_sum + (kernel_area / 2)) / kernel_area);
    }

    for (int y = kernel_size; y < height; y++) {
      src_pixel_offset = src_col_offset + y * src_stride;
      dst_pixel_offset = dst_col_offset + y * dst_stride;

      int leave_offset = src_col_offset + (y - kernel_size) * src_stride;
      window_sum = window_sum + src[src_pixel_offset] - src[leave_offset];
      dst[dst_pixel_offset] =
          (uint8_t)((window_sum + (kernel_area / 2)) / kernel_area);
    }
  }
}