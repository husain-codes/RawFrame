#include <img/img_processing.h>
#include <img/isp_pipeline.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

isp_context_t *isp_create(void) {
  printf("[ISP_API] %s()\n", __func__);
  isp_context_t *ctx = (isp_context_t *)calloc(1, sizeof(isp_context_t));
  if (!ctx) {
    printf(" -> ERROR: Memory allocation failed!\n");
    return NULL;
  }
  ctx->state = ISP_STATE_UNINIT;
  return ctx;
}

int isp_init(isp_context_t *ctx, const isp_config_t *config) {
  printf("[ISP_API] %s()\n", __func__);
  if (!ctx || !config)
    return -1;

  if (ctx->state != ISP_STATE_UNINIT) {
    printf(" -> ERROR: ISP Engine already initialized!\n");
    return -1;
  }

  if (config->width == 0 || config->height == 0) {
    printf(" -> ERROR: Invalid dimensions!\n");
    ctx->state = ISP_STATE_ERROR;
    return -1;
  }

  if (config->input_format == IMG_FMT_UNKNOWN ||
      config->output_format == IMG_FMT_UNKNOWN) {
    printf(" -> ERROR: Invalid image format!\n");
    ctx->state = ISP_STATE_ERROR;
    return -1;
  }

  if (config->width & 1 || config->height & 1) {
    printf(" -> ERROR: Width and Height must be even for Bayer 2x2 grid!\n");
    ctx->state = ISP_STATE_ERROR;
    return -1;
  }

  ctx->config = *config;
  ctx->r_gain = 1.0f;
  ctx->g_gain = 1.0f;
  ctx->b_gain = 1.0f;
  ctx->state = ISP_STATE_CONFIGURED;

  // Initialize Gamma LUT to identity mapping
  for(int i=0; i<256; i++) {
    float val = i/255.0;
    ctx->gamma_lut[i] = (uint8_t)(pow(val, 1.0f / 2.2f) * 255.0f + 0.5f);
  }
  return 0;
}

int isp_start(isp_context_t *ctx) {
  printf("[ISP_API] %s()\n", __func__);
  if (!ctx || ctx->state != ISP_STATE_CONFIGURED)
    return -1;
  ctx->state = ISP_STATE_RUNNING;
  return 0;
}

int isp_stop(isp_context_t *ctx) {
  printf("[ISP_API] %s()\n", __func__);
  if (!ctx)
    return -1;
  ctx->state = ISP_STATE_CONFIGURED;
  return 0;
}

void isp_destroy(isp_context_t *ctx) {
  printf("[ISP_API] %s()\n", __func__);
  if (ctx) {
    ctx->state = ISP_STATE_UNINIT;
    free(ctx);
  }
}

/* Stubs for each Pass */
int isp_pass_black_level_subtraction(isp_context_t *ctx, uint16_t *frame) {
  printf("  [PASS] %s()\n", __func__);
  if (!ctx || !frame) {
    printf("%s: Input is NULL\n", __func__);
    return -1;
  }

  // check format is raw bayer 10-bit (IMG_FMT_BAYER_10BIT)
  if (ctx->config.input_format != IMG_FMT_BAYER_10BIT) {
    printf("%s: Unsupported input format (Expected IMG_FMT_BAYER_10BIT)\n",
           __func__);
    return -1;
  }

  if (frame == NULL) {
    printf("%s: NULL frame\n", __func__);
    return -1;
  }

  uint32_t total_pixel = ctx->config.height * ctx->config.width;
  uint16_t *pixel = frame;
  uint16_t black_level = ctx->config.black_level;
  for (size_t i = 0; i < total_pixel; i++) {
    if (pixel[i] > black_level) {
      pixel[i] -= (uint8_t)black_level;
    } else {
      pixel[i] = 0;
    }
  }
  return 0;
}

int isp_pass_auto_white_balance_stats(isp_context_t *ctx,
                                      const uint16_t *frame) {
  printf("  [PASS] %s()\n", __func__);
  if (!ctx || !frame) {
    printf(" -> ERROR: NULL pointer passed to %s\n", __func__);
    return -1;
  }

  if (ctx->config.input_format != IMG_FMT_BAYER_10BIT) {
    printf(
        " -> ERROR: Unsupported Bayer format in %s (Currently expects RGGB)\n",
        __func__);
    return -1;
  }

  uint32_t width = ctx->config.width;
  uint32_t height = ctx->config.height;
  const uint16_t *pixel = frame;
  size_t sum_r = 0, sum_g = 0, sum_b = 0;
  size_t count_r = 0, count_g = 0, count_b = 0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint16_t value = pixel[y * width + x];
      if (!(y & 1) && !(x & 1)) {
        count_r++;
        sum_r += value;
      } else if ((y & 1) && (x & 1)) {
        sum_b += value;
        count_b++;
      } else {
        sum_g += value;
        count_g++;
      }
    }
  }

  if (count_r == 0 || count_b == 0 || count_g == 0) {
    printf(" -> ERROR: No pixels found for one of the channels!\n");
    return -1;
  }
  float avg_r = (float)sum_r / count_r;
  float avg_g = (float)sum_g / count_g;
  float avg_b = (float)sum_b / count_b;

  // Prevent division by zero if a channel is completely dark
  if (avg_r < 1.0f)
    avg_r = 1.0f;
  if (avg_g < 1.0f)
    avg_g = 1.0f;
  if (avg_b < 1.0f)
    avg_b = 1.0f;

  ctx->r_gain = avg_g / avg_r;
  ctx->g_gain = 1.0f;
  ctx->b_gain = avg_g / avg_b;
  return 0;
}

int isp_pass_apply_white_balance(isp_context_t *ctx, uint16_t *frame) {
  printf("  [PASS] %s()\n", __func__);
  if (!ctx || !frame) {
    printf(" -> ERROR: NULL pointer passed to %s\n", __func__);
    return -1;
  }

  if (ctx->config.input_format != IMG_FMT_BAYER_10BIT) {
    printf(" -> ERROR: Unsupported Bayer format in %s\n", __func__);
    return -1;
  }

  uint32_t width = ctx->config.width;
  uint32_t height = ctx->config.height;
  uint16_t *pixel = frame;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      size_t idx = y * width + x;
      uint16_t value = pixel[idx];
      if (!(y & 1) && !(x & 1)) {
        // Red pixel
        float new_val = value * ctx->r_gain;
        // printf(" -> DEBUG: R pixel at (%d,%d): %u * %.2f = %.2f\n", x, y,
        // value, ctx->r_gain, new_val);
        if (new_val > 1023.0f)
          new_val = 1023.0f;
        pixel[idx] = (uint16_t)(new_val + 0.5f);
      } else if ((y & 1) && (x & 1)) {
        // Blue pixel
        float new_val = value * ctx->b_gain;
        if (new_val > 1023.0f)
          new_val = 1023.0f;
        pixel[idx] = (uint16_t)(new_val + 0.5f);
      }
    }
  }
  return 0;
}

int isp_pass_demosaic(isp_context_t *ctx, const img_t *src, img_t *dst) {
  printf("  [PASS] %s()\n", __func__);
  if (!ctx || !src || !dst) {
    printf(" -> ERROR: NULL pointer passed to %s\n", __func__);
    return -1;
  }
  uint32_t width = src->width;
  uint32_t height = src->height;
  if (dst->width != width || dst->height != height) {
    printf(" -> ERROR: source and destination metadata are different %s\n",
           __func__);
    return -1;
  }

  if (src->format != IMG_FMT_BAYER_RGGB) {
    printf(" -> ERROR: src image format must be BAYER_RGGB in %s\n", __func__);
    return -1;
  }

  if (dst->format != IMG_FMT_RGB24) {
    printf(" -> ERROR:  dst image format should be RGB24 %s\n", __func__);
    return -1;
  }
  int bpp = img_format_bytes_per_pixel(dst->format);
  const uint8_t *src_pixel = src->planes[0];
  uint8_t *dst_pixel = dst->planes[0];

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      size_t src_idx = y * width + x;
      uint8_t value = src_pixel[src_idx];
      dst_pixel = dst->planes[0] + y * dst->stride[0] + x * bpp;

      int left = (x > 0) ? x - 1 : x;
      int right = (x < width - 1) ? x + 1 : x;
      int top = (y > 0) ? y - 1 : y;
      int bot = (y < height - 1) ? y + 1 : y;

      if (!(y & 1) && !(x & 1)) {
        // Red Pixel
        dst_pixel[0] = value; // R
        uint32_t g_sum =
            src_pixel[y * width + right] + src_pixel[y * width + left] +
            src_pixel[top * width + x] + src_pixel[bot * width + x];
        uint32_t b_sum =
            src_pixel[bot * width + right] + src_pixel[top * width + right] +
            src_pixel[bot * width + left] + src_pixel[top * width + left];
        dst_pixel[1] =
            (uint8_t)((g_sum + 2) /
                      4); // Adding 2 gives nearest-integer rounding--> Green
        dst_pixel[2] =
            (uint8_t)((b_sum + 2) /
                      4); // Adding 2 gives nearest-integer rounding--> Blue

      } else if ((y & 1) && (x & 1)) {
        // Blue pixel
        dst_pixel[2] = value; // B
        uint32_t g_sum =
            src_pixel[top * width + x] + src_pixel[bot * width + x] +
            src_pixel[y * width + right] + src_pixel[y * width + left];

        uint32_t r_sum =
            src_pixel[top * width + left] + src_pixel[top * width + right] +
            src_pixel[bot * width + right] + src_pixel[bot * width + left];

        dst_pixel[0] =
            (uint8_t)((r_sum + 2) /
                      4); // Adding 2 gives nearest-integer rounding--> Red
        dst_pixel[1] =
            (uint8_t)((g_sum + 2) /
                      4); // Adding 2 gives nearest-integer rounding--> Green
      } else {
        // green pixel
        dst_pixel[1] = value; // G
        if (y & 1) {
          // Green pixel on Blue row
          uint32_t b_sum =
              src_pixel[y * width + right] + src_pixel[y * width + left];
          uint32_t r_sum =
              src_pixel[top * width + x] + src_pixel[bot * width + x];
          dst_pixel[2] =
              (uint8_t)((b_sum + 1) /
                        2); // Adding 1 gives nearest-integer rounding--> Blue
          dst_pixel[0] =
              (uint8_t)((r_sum + 1) /
                        2); // Adding 1 gives nearest-integer rounding--> Red
        } else {
          // Green pixel on Red row
          uint32_t r_sum =
              src_pixel[y * width + right] + src_pixel[y * width + left];
          uint32_t b_sum =
              src_pixel[top * width + x] + src_pixel[bot * width + x];
          dst_pixel[0] =
              (uint8_t)((r_sum + 1) /
                        2); // Adding 1 gives nearest-integer rounding--> Red
          dst_pixel[2] =
              (uint8_t)((b_sum + 1) /
                        2); // Adding 1 gives nearest-integer rounding--> Blue
        }
      }
    }
  }
  return 0;
}

int isp_convert_16bitraw_to_img(isp_context_t *ctx, const uint16_t *src,
                                img_t *dst) {
  printf("  [PASS] %s()\n", __func__);
  if (!ctx || !src || !dst) {
    printf(" -> ERROR: NULL pointer passed to %s\n", __func__);
    return -1;
  }

  if (ctx->config.input_format != IMG_FMT_BAYER_10BIT) {
    printf(" -> ERROR: Unsupported Bayer format in %s\n", __func__);
    return -1;
  }

  if (dst->format != IMG_FMT_BAYER_RGGB) {
    printf(" -> ERROR: dst image format should be BAYER_RGGB %s\n", __func__);
    return -1;
  }

  // Dimensions check
  if (dst->width != ctx->config.width || dst->height != ctx->config.height) {
    printf(
        " -> ERROR: dst image dimensions do not match context config in %s\n",
        __func__);
    return -1;
  }

  uint32_t total_pixels = ctx->config.width * ctx->config.height;
  uint8_t *dst_pixel = dst->planes[0];

  for (uint32_t i = 0; i < total_pixels; i++) {
    uint16_t value = src[i];
    if (value > 1023) {
      value = 1023; // Clamp to 10-bit max
    }
    dst_pixel[i] = (uint8_t)(value >> 2); // Convert to 8-bit by shifting
  }
  return 0;
}

int isp_pass_blur(isp_context_t *ctx, const img_t *src, img_t *dst) {
  printf("  [PASS] %s()\n", __func__);
  return 0;
}

int isp_pass_gamma_correction(isp_context_t *ctx, img_t *img) {
  printf("  [PASS] %s()\n", __func__);
  if (!ctx || !img) {
    printf(" -> ERROR: NULL pointer passed to %s\n", __func__);
    return -1;
  }
  uint32_t width = img->width;
  uint32_t height = img->height;

  if (img->format != IMG_FMT_RGB24) {
    printf(" -> ERROR: src image format must be RGB24 in %s\n", __func__);
    return -1;
  }

  int bpp = img_format_bytes_per_pixel(img->format);
  uint8_t *pixel;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      pixel = img->planes[0] +y * img->stride[0] + x * bpp;
      pixel[0] = ctx->gamma_lut[pixel[0]];
      pixel[1] = ctx->gamma_lut[pixel[1]];
      pixel[2] = ctx->gamma_lut[pixel[2]];
    }
  }
  return 0;
}

/* Orchestrator Function */
int isp_process_frame(isp_context_t *ctx, const uint8_t *src, uint16_t *dst,
                      img_t *out_rgb, int frame_index) {
  printf("[ISP_PIPELINE] %s() [Frame #%u]\n", __func__, frame_index);

  if (!ctx || ctx->state != ISP_STATE_RUNNING) {
    printf(" -> ERROR: ISP Engine not running!\n");
    return -1;
  }

  if (!src || !dst || !out_rgb) {
    printf(" -> ERROR: NULL pointer passed to %s\n", __func__);
    return -1;
  }

  unpack_10bit_mipi(src, dst, ctx->config.width, ctx->config.height,
                    ctx->config.stride);
  // unpack 10-bit MIPI data to 16-bit for processing

  if (ctx->config.enable_bls) {
    isp_pass_black_level_subtraction(ctx, dst);
  }

  if (ctx->config.enable_awb) {
    isp_pass_auto_white_balance_stats(ctx, dst);
    isp_pass_apply_white_balance(ctx, dst);
  }

  // Create an 8-bit Bayer image from the 16-bit raw data for further processing
  img_t *bayer_8bit_img =
      img_create(ctx->config.width, ctx->config.height, IMG_FMT_BAYER_RGGB);
  if (!bayer_8bit_img) {
    printf(" -> ERROR: Failed to create 8-bit Bayer image for processing!\n");
    return -1;
  }
  // convert 16-bit raw to 8-bit Bayer image
  if (isp_convert_16bitraw_to_img(ctx, dst, bayer_8bit_img) < 0) {
    printf(" -> ERROR: Failed to convert 16-bit raw to 8-bit Bayer image!\n");
    img_destroy(bayer_8bit_img);
    return -1;
  }

  if (ctx->config.enable_demosaic) {
    isp_pass_demosaic(ctx, bayer_8bit_img, out_rgb);
  }

  if(ctx->config.enable_gamma_correction) {
    isp_pass_gamma_correction(ctx, out_rgb);
  }

  if (ctx->config.enable_blur) {
    isp_pass_blur(ctx, out_rgb, out_rgb);
  }

  // Clean up temporary images
  img_destroy(bayer_8bit_img);
  return 0;
}
