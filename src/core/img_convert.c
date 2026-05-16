#include <img/img.h>
#include <img/img_convert.h>
#include <img/img_format.h>
#include <stdio.h>
#include <stdlib.h>

#define CLAMP(x) (x < 0 ? 0 : (x > 255 ? 255 : (uint8_t)x))

/* Convert RGB image to BGR
 * This function takes an image in RGB or BGR format and toggles it.
 * Parameters:  - img: A pointer to the image structure that contains the RGB or
 * BGR image data. The function modifies this image in place. Returns:  - 0 on
 * success (If the input image is toggled), -1 on failure (e.g., if the input
 * image is null or img isn't RGB or BGR).
 */
int img_toggle_rgb_bgr(img_t *img) {
  if (!img) {
    fprintf(stderr, "Image is null\n");
    return -1;
  }
  if (img->format != IMG_FMT_BGR24 && img->format != IMG_FMT_RGB24) {
    fprintf(stderr, "Image format is not RGB or BGR\n");
    return -1;
  }

  uint8_t *pixel;
  uint8_t *row_offset;
  uint8_t temp = 0;
  for (int y = 0; y < img->height; y++) {
    row_offset = img->planes[0] + y * img->stride[0];
    for (int x = 0; x < img->width; x++) {
      pixel = row_offset + x * 3;
      temp = pixel[0];     // getting red value into temp var
      pixel[0] = pixel[2]; // assign blue value to first pixel
      pixel[2] = temp;     // assign red value to last pixel
    }
  }

  if (img->format == IMG_FMT_BGR24) {
    img->format = IMG_FMT_RGB24;
  } else {
    img->format = IMG_FMT_BGR24;
  }

  return 0;
}

/* Convert RGB image to Grayscale
 * This function converts an RGB image to grayscale using the luminosity method.
 * Parameters:  - img: A pointer to the image structure that contains the RGB
 * image data. The function modifies this image in place, changing its format to
 * grayscale. it is lossy conversion, so the original RGB data will be
 * discarded. this a irreversible operation. Once converted to grayscale, the
 * image cannot be converted back to RGB with the original colors. Returns:  - 0
 * on success (If the input RGB image is successfully converted to grayscale),
 * -1 on failure (e.g., if the input image is null or not in RGB format). Note:
 * Function expects the input image to be in RGB format. If the image is in BGR
 * format, it should be toggled to RGB first using img_toggle_rgb_bgr before
 * calling this function.
 */
int img_convert_rgb_to_gray(img_t *img) {
  if (!img) {
    fprintf(stderr, "Image is null\n");
    return -1;
  }

  if (img->format != IMG_FMT_RGB24) {
    fprintf(stderr, "Image format is not RGB\n");
    return -1;
  }
  size_t buffer_size = img->width * img->height;
  uint8_t *new_data = (uint8_t *)malloc(buffer_size);
  if (!new_data) {
    fprintf(stderr, "Unable to allocate memory of size %zu\n", buffer_size);
    return -1;
  }

  uint8_t *p;      // pointer to the current pixel in the original RGB image
  uint8_t *gray_p; // pointer to the current pixel in the new grayscale image
  uint8_t *row_offset;      // pointer to the start of the current row in the
                            // original RGB image
  uint8_t *row_offset_gray; // pointer to the start of the current row in the
                            // new grayscale image
  size_t bpp = img_format_bytes_per_pixel(img->format);
  for (int y = 0; y < img->height; y++) {
    row_offset = img->planes[0] + y * img->stride[0];
    row_offset_gray = new_data + y * img->width;
    for (int x = 0; x < img->width; x++) {
      p = row_offset + x * bpp;
      gray_p = row_offset_gray + x;
      gray_p[0] = (uint8_t)(0.299 * p[0] + 0.587 * p[1] + 0.114 * p[2]);
    }
  }

  free(img->planes[0]);
  img->planes[0] = new_data;
  img->format = IMG_FMT_GRAY8;
  img->stride[0] = img->width;
  return 0;
}

/* Convert RGB image to YUV444
 * This function converts an RGB image to YUV444 format using the standard
 * conversion formula. Parameters:  - img: A pointer to the image structure that
 * contains the RGB image data. The function modifies this image in place,
 * changing its format to YUV444. Returns:  - 0 on success (If the input RGB
 * image is successfully converted to YUV444), -1 on failure (e.g., if the input
 * image is null or not in RGB format). Note: Function expects the input image
 * to be in RGB format. If the image is in BGR format, it should be toggled to
 * RGB first using img_toggle_rgb_bgr before calling this function.
 */
int img_convert_rgb_to_yuv444(img_t *img) {
  if (!img) {
    fprintf(stderr, "Image is null \n");
    return -1;
  }

  if (img->format != IMG_FMT_RGB24) {
    fprintf(stderr, "Image format is not RGB\n");
    return -1;
  }

  uint8_t *pixel;
  uint8_t *row_offset;
  uint8_t Y, U, V;
  size_t bpp = img_format_bytes_per_pixel(img->format);
  for (int y = 0; y < img->height; y++) {
    row_offset = img->planes[0] + y * img->stride[0];
    for (int x = 0; x < img->width; x++) {
      pixel = row_offset + x * bpp;
      Y = (uint8_t)(0.299 * pixel[0] + 0.587 * pixel[1] + 0.114 * pixel[2]);
      U = (uint8_t)(-0.169 * pixel[0] - 0.331 * pixel[1] + 0.5 * pixel[2] +
                    128);
      V = (uint8_t)(0.5 * pixel[0] - 0.419 * pixel[1] - 0.081 * pixel[2] + 128);
      pixel[0] = Y;
      pixel[1] = U;
      pixel[2] = V;
    }
  }
  img->format = IMG_FMT_YUV444;
  return 0;
}

/* Convert YUV444 image to RGB
 * This function converts a YUV444 image back to RGB format using the standard
 * conversion formula. Parameters:  - img: A pointer to the image structure that
 * contains the YUV444 image data. The function modifies this image in place,
 * changing its format to RGB. Returns:  - 0 on success (If the input YUV444
 * image is successfully converted to RGB), -1 on failure (e.g., if the input
 * image is null or not in YUV444 format).
 */
int img_convert_yuv444_to_rgb(img_t *img) {
  if (!img) {
    fprintf(stderr, "Image is null \n");
    return -1;
  }

  if (img->format != IMG_FMT_YUV444) {
    fprintf(stderr, "Image format is not RGB\n");
    return -1;
  }

  uint8_t *pixel;
  uint8_t *row_offset;
  uint8_t R, G, B;
  double r_val, g_val, b_val;
  size_t bpp = img_format_bytes_per_pixel(img->format);
  for (int y = 0; y < img->height; y++) {
    row_offset = img->planes[0] + y * img->stride[0];
    for (int x = 0; x < img->width; x++) {
      pixel = row_offset + x * bpp;
      r_val = pixel[0] + 1.402 * (pixel[2] - 128); // Y + 1.402 * (V - 128)
      g_val =
          pixel[0] - 0.344 * (pixel[1] - 128) -
          0.714 * (pixel[2] - 128); // Y - 0.344 * (U - 128) - 0.714 * (V - 128)
      b_val = pixel[0] + 1.772 * (pixel[1] - 128); // Y + 1.772 * (U - 128)
      R = r_val < 0 ? 0 : (r_val > 255 ? 255 : (uint8_t)r_val);
      G = g_val < 0 ? 0 : (g_val > 255 ? 255 : (uint8_t)g_val);
      B = b_val < 0 ? 0 : (b_val > 255 ? 255 : (uint8_t)b_val);
      pixel[0] = R;
      pixel[1] = G;
      pixel[2] = B;
    }
  }

  img->format = IMG_FMT_RGB24;
  return 0;
}
/*
 * Convert RGB image to NV12
 * This function converts an RGB image to NV12 format.
 * Parameters:
 *   img: A pointer to the input RGB image
 * Returns:
 *   A pointer to the new NV12 image on success, or NULL on failure
 */
img_t *img_convert_rgb_to_nv12(const img_t *img) {
  if (!img) {
    fprintf(stderr, "Input image is null\n");
    return NULL;
  }

  if (img->format != IMG_FMT_RGB24) {
    fprintf(stderr, "Input image format is %d, expected RGB24\n", img->format);
    return NULL;
  }

  if (img->width % 2 != 0 || img->height % 2 != 0) {
    fprintf(stderr, "Input image dimensions must be even for NV12 format\n");
    return NULL;
  }

  img_t *img_nv12 = img_create(img->width, img->height, IMG_FMT_NV12);
  if (!img_nv12) {
    fprintf(stderr, "Failed to create NV12 image\n");
    return NULL;
  }

  int bpp = img_format_bytes_per_pixel(img->format);
  uint8_t *row_offset_rgb;
  uint8_t *row_offset_y;
  uint8_t *row_offset_uv;
  uint8_t *pixel_rgb;
  uint8_t *pixel_y;
  uint8_t *pixel_uv;

  for (int y = 0; y < img->height; y++) {
    row_offset_rgb = img->planes[0] + y * img->stride[0];
    row_offset_y = img_nv12->planes[0] + y * img_nv12->stride[0];
    for (int x = 0; x < img->width; x++) {
      pixel_rgb = row_offset_rgb + x * bpp;
      pixel_y = row_offset_y + x; // Y plane has 1 byte per pixel
      pixel_y[0] = (uint8_t)(0.299 * pixel_rgb[0] + 0.587 * pixel_rgb[1] +
                             0.114 * pixel_rgb[2]);
    }
  }

  for (int y = 0; y < img->height; y += 2) {
    row_offset_rgb = img->planes[0] + y * img->stride[0];
    row_offset_uv = img_nv12->planes[1] + (y / 2) * img_nv12->stride[1];
    for (int x = 0; x < img->width; x += 2) {
      pixel_uv = row_offset_uv + (x / 2) * 2;
      uint8_t *pixel_tl = row_offset_rgb + x * bpp;
      uint8_t *pixel_tr = row_offset_rgb + (x + 1) * bpp;
      uint8_t *pixel_bl = row_offset_rgb + img->stride[0] + x * bpp;
      uint8_t *pixel_br = row_offset_rgb + img->stride[0] + (x + 1) * bpp;

      uint16_t r_sum = pixel_tl[0] + pixel_tr[0] + pixel_bl[0] + pixel_br[0];
      uint16_t g_sum = pixel_tl[1] + pixel_tr[1] + pixel_bl[1] + pixel_br[1];
      uint16_t b_sum = pixel_tl[2] + pixel_tr[2] + pixel_bl[2] + pixel_br[2];

      uint8_t r_avg = (uint8_t)(r_sum / 4);
      uint8_t g_avg = (uint8_t)(g_sum / 4);
      uint8_t b_avg = (uint8_t)(b_sum / 4);

      // Average the U and V values for the 2x2 block of pixels
      uint8_t U = (uint8_t)(-0.169 * r_avg - 0.331 * g_avg + 0.5 * b_avg + 128);
      uint8_t V = (uint8_t)(0.5 * r_avg - 0.419 * g_avg - 0.081 * b_avg + 128);

      pixel_uv[0] = U;
      pixel_uv[1] = V;
    }
  }
  return img_nv12;
}

/* Convert NV12 image to RGB
 * This function converts an NV12 image back to RGB format.
 * Parameters:
 *   img: A pointer to the input NV12 image
 * Returns:
 *   A pointer to the new RGB image on success, or NULL on failure
 */
img_t *img_convert_nv12_to_rgb(const img_t *img) {
  if (!img) {
    fprintf(stderr, "Input image is null\n");
    return NULL;
  }
  if (img->format != IMG_FMT_NV12) {
    fprintf(stderr, "Input image format is %d, expected NV12\n", img->format);
    return NULL;
  }
  img_t *img_rgb = img_create(img->width, img->height, IMG_FMT_RGB24);
  if (!img_rgb) {
    fprintf(stderr, "Failed to create RGB image\n");
    return NULL;
  }

  uint8_t *row_offset_y;
  uint8_t *row_offset_uv;
  uint8_t *row_offset_rgb;
  int bpp = img_format_bytes_per_pixel(img_rgb->format);
  for (int y = 0; y < img->height; y++) {
    row_offset_y = img->planes[0] + y * img->stride[0];
    row_offset_uv = img->planes[1] + (y / 2) * img->stride[1];
    row_offset_rgb = img_rgb->planes[0] + y * img_rgb->stride[0];
    for (int x = 0; x < img->width; x++) {
      uint8_t *Y = row_offset_y + x;

      uint8_t *U = row_offset_uv + (x / 2) * 2;
      uint8_t *V = row_offset_uv + (x / 2) * 2 + 1;

      double r_val = *Y + 1.402 * (*V - 128);
      double g_val = *Y - 0.344 * (*U - 128) - 0.714 * (*V - 128);
      double b_val = *Y + 1.772 * (*U - 128);

      uint8_t *pixel = row_offset_rgb + x * bpp;

      uint8_t R = r_val < 0 ? 0 : (r_val > 255 ? 255 : (uint8_t)r_val);
      uint8_t G = g_val < 0 ? 0 : (g_val > 255 ? 255 : (uint8_t)g_val);
      uint8_t B = b_val < 0 ? 0 : (b_val > 255 ? 255 : (uint8_t)b_val);

      pixel[0] = R;
      pixel[1] = G;
      pixel[2] = B;
    }
  }
  return img_rgb;
}

/*
 * Convert RGB image to YUV420P
 * This function converts an RGB image to YUV420P format.
 * Parameters:
 *   img: A pointer to the input RGB image
 * Returns:
 *   A pointer to the new YUV420P image on success, or NULL on failure
 */
img_t *img_convert_rgb_to_yuv420p(const img_t *img) {
  if (!img) {
    fprintf(stderr, "Input image is null\n");
    return NULL;
  }

  if (img->format != IMG_FMT_RGB24) {
    fprintf(stderr, "Input image format is %d, expected RGB24\n", img->format);
    return NULL;
  }

  if (img->width % 2 != 0 || img->height % 2 != 0) {
    fprintf(stderr, "Input image dimensions must be even for YUV420p format\n");
    return NULL;
  }

  img_t *img_yuv420p = img_create(img->width, img->height, IMG_FMT_YUV420P);
  if (!img_yuv420p) {
    fprintf(stderr, "Failed to create YUV420P image\n");
    return NULL;
  }

  int bpp = img_format_bytes_per_pixel(img->format);
  uint8_t *row_offset_rgb;
  uint8_t *row_offset_y;
  uint8_t *row_offset_u;
  uint8_t *row_offset_v;
  uint8_t *pixel_rgb;
  uint8_t *pixel_y;

  for (int y = 0; y < img->height; y++) {
    row_offset_rgb = img->planes[0] + y * img->stride[0];
    row_offset_y = img_yuv420p->planes[0] + y * img_yuv420p->stride[0];
    for (int x = 0; x < img->width; x++) {
      pixel_rgb = row_offset_rgb + x * bpp;
      pixel_y = row_offset_y + x;
      pixel_y[0] = (uint8_t)(0.299 * pixel_rgb[0] + 0.587 * pixel_rgb[1] +
                             0.114 * pixel_rgb[2]);
    }
  }

  for (int y = 0; y < img->height; y += 2) {
    row_offset_rgb = img->planes[0] + y * img->stride[0];
    row_offset_u = img_yuv420p->planes[1] + (y / 2) * img_yuv420p->stride[1];
    row_offset_v = img_yuv420p->planes[2] + (y / 2) * img_yuv420p->stride[2];
    for (int x = 0; x < img->width; x += 2) {
      uint8_t *pixel_u = row_offset_u + (x / 2);
      uint8_t *pixel_v = row_offset_v + (x / 2);

      uint8_t *pixel_tl = row_offset_rgb + x * bpp;
      uint8_t *pixel_tr = row_offset_rgb + (x + 1) * bpp;
      uint8_t *pixel_bl = row_offset_rgb + img->stride[0] + x * bpp;
      uint8_t *pixel_br = row_offset_rgb + img->stride[0] + (x + 1) * bpp;

      uint16_t r_sum = pixel_tl[0] + pixel_tr[0] + pixel_bl[0] + pixel_br[0];
      uint16_t g_sum = pixel_tl[1] + pixel_tr[1] + pixel_bl[1] + pixel_br[1];
      uint16_t b_sum = pixel_tl[2] + pixel_tr[2] + pixel_bl[2] + pixel_br[2];

      uint8_t r_avg = (uint8_t)(r_sum / 4);
      uint8_t g_avg = (uint8_t)(g_sum / 4);
      uint8_t b_avg = (uint8_t)(b_sum / 4);

      // Average the U and V values for the 2x2 block of pixels
      uint8_t U = (uint8_t)(-0.169 * r_avg - 0.331 * g_avg + 0.5 * b_avg + 128);
      uint8_t V = (uint8_t)(0.5 * r_avg - 0.419 * g_avg - 0.081 * b_avg + 128);

      pixel_u[0] = U;
      pixel_v[0] = V;
    }
  }
  return img_yuv420p;
}

/*
 * Convert YUV420P image to RGB
 * This function converts a YUV420P image back to RGB format.
 * Parameters:
 *   img: A pointer to the input YUV420P image
 * Returns:
 *   A pointer to the new RGB image on success, or NULL on failure
 */
img_t *img_convert_yuv420p_to_rgb(const img_t *img) {
  if (!img) {
    fprintf(stderr, "Input image is null\n");
    return NULL;
  }

  if (img->format != IMG_FMT_YUV420P) {
    fprintf(stderr, "Input image format is %d, expected YUV420P\n",
            img->format);
    return NULL;
  }

  img_t *img_rgb = img_create(img->width, img->height, IMG_FMT_RGB24);

  if (!img_rgb) {
    fprintf(stderr, "Failed to create RGB image\n");
    return NULL;
  }

  uint8_t *row_offset_y;
  uint8_t *row_offset_u;
  uint8_t *row_offset_v;
  uint8_t *row_offset_rgb;
  int bpp = img_format_bytes_per_pixel(img_rgb->format);

  for (int y = 0; y < img->height; y++) {
    row_offset_y = img->planes[0] + y * img->stride[0];
    row_offset_u = img->planes[1] + (y / 2) * img->stride[1];
    row_offset_v = img->planes[2] + (y / 2) * img->stride[2];
    row_offset_rgb = img_rgb->planes[0] + y * img_rgb->stride[0];
    for (int x = 0; x < img->width; x++) {
      uint8_t *Y = row_offset_y + x;
      uint8_t *U = row_offset_u + x / 2;
      uint8_t *V = row_offset_v + x / 2;

      double r_val = *Y + 1.402 * (*V - 128);
      double g_val = *Y - 0.344 * (*U - 128) - 0.714 * (*V - 128);
      double b_val = *Y + 1.772 * (*U - 128);

      uint8_t *pixel = row_offset_rgb + x * bpp;

      uint8_t R = CLAMP(r_val);
      uint8_t G = CLAMP(g_val);
      uint8_t B = CLAMP(b_val);

      pixel[0] = R;
      pixel[1] = G;
      pixel[2] = B;
    }
  }
  return img_rgb;
}
