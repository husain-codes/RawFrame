#include <img/img.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Function to create an image of width x height.
 * return NULL on failure.
 * return pointer to img_t on success.
 */
img_t *img_create(uint32_t width, uint32_t height, img_format_t format) {
  if (!width || !height)
    return NULL;
  size_t bpp = img_format_bytes_per_pixel(format);

  // Important checks.
  if (bpp == 0)
    return NULL;
  if (width > SIZE_MAX / bpp)
    return NULL;
  if (height > SIZE_MAX / (width * bpp))
    return NULL;

  img_t *img = (img_t *)malloc(sizeof(img_t));
  if (!img)
    return NULL;

  img->width = width;
  img->height = height;
  img->format = format;
  img->stride =
      width * bpp; // number of bytes in a row of pixel data, excluding padding
  size_t buffer_size = img->stride * height;
  img->data = malloc(buffer_size);
  if (!img->data) {
    free(img);
    return NULL;
  }
  memset(img->data, 0, buffer_size); // set 0 as initial value
  return img;
}

/* Function to destroy image */
void img_destroy(img_t *img) {
  if (!img)
    return;
  free(img->data);
  free(img);
}

/* Function to get pixel value at (x, y) and store it in out.
 * return 0 on success.
 * return -1 on failure.
 */
int img_get_pixel(img_t *img, int x, int y, pixel_t *out) {
  if (!img || !out || x < 0 || y < 0 || x >= img->width || y >= img->height) {
    fprintf(stderr, "Invalid input to img_get_pixel\n");
    return -1;
  }
  size_t bpp = img_format_bytes_per_pixel(img->format);
  if (bpp == 0) {
    fprintf(stderr, "Unsupported image format\n");
    return -1;
  }
  uint8_t *row = img->data + y * img->stride;
  uint8_t *pixel_data = row + x * bpp;
  switch (img->format) {
  case IMG_FMT_GRAY8:
    out->r = out->g = out->b = pixel_data[0];
    out->a = 255;
    break;
  case IMG_FMT_RGB24:
    out->r = pixel_data[0];
    out->g = pixel_data[1];
    out->b = pixel_data[2];
    out->a = 255;
    break;
  case IMG_FMT_BGR24:
    out->b = pixel_data[0];
    out->g = pixel_data[1];
    out->r = pixel_data[2];
    out->a = 255;
    break;
  case IMG_FMT_RGBA32:
    out->r = pixel_data[0];
    out->g = pixel_data[1];
    out->b = pixel_data[2];
    out->a = pixel_data[3];
    break;
  default:
    fprintf(stderr, "Unsupported image format\n");
    return -1;
  }
  return 0;
}

/* Function to set pixel value at (x, y) to p.
 * return 0 on success.
 * return -1 on failure.
 */
int img_set_pixel(img_t *img, int x, int y, pixel_t p) {
  if (!img || x < 0 || y < 0 || x >= img->width || y >= img->height) {
    fprintf(stderr, "Invalid input to img_set_pixel\n");
    return -1;
  }
  size_t bpp = img_format_bytes_per_pixel(img->format);
  if (bpp == 0) {
    fprintf(stderr, "Unsupported image format\n");
    return -1;
  }
  uint8_t *row = img->data + y * img->stride;
  uint8_t *pixel_data = row + x * bpp;
  switch (img->format) {
  case IMG_FMT_GRAY8:
    pixel_data[0] = 0.299 * p.r + 0.587 * p.g + 0.114 * p.b;
    break;
  case IMG_FMT_RGB24:
    pixel_data[0] = p.r;
    pixel_data[1] = p.g;
    pixel_data[2] = p.b;
    break;
  case IMG_FMT_BGR24:
    pixel_data[0] = p.b;
    pixel_data[1] = p.g;
    pixel_data[2] = p.r;
    break;
  case IMG_FMT_RGBA32:
    pixel_data[0] = p.r;
    pixel_data[1] = p.g;
    pixel_data[2] = p.b;
    pixel_data[3] = p.a;
    break;
  default:
    fprintf(stderr, "Unsupported image format\n");
    return -1;
  }
  return 0;
}