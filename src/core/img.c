#include <img/img.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Function to create an nv12 image of width x height.
 * return NULL on failure.
 * return pointer to img_t on success.
 */
static img_t *img_create_nv12(uint32_t width, uint32_t height) {
  if (width > SIZE_MAX || height > SIZE_MAX)
    return NULL;

  img_t *img = (img_t *)malloc(sizeof(img_t));
  if (!img)
    return NULL;
  img->width = width;
  img->height = height;
  img->num_planes = 2; // NV12 has 2 planes;
  img->format = IMG_FMT_NV12;
  img->stride[0] = width;
  img->stride[1] = width;
  img->stride[2] = 0; // since nv12 has only 2 planes.

  size_t buffer_size_Y_plane = width * height; // for Y plane.

  // plane 0 will act as Y plane
  img->planes[0] = malloc(buffer_size_Y_plane);
  if (!img->planes[0]) {
    free(img);
    return NULL;
  }

  // plane 1 will act as UV interleaved plane
  size_t buffer_size_uv_plane =
      buffer_size_Y_plane / 2; // since UV plane is half of Y plane.
  img->planes[1] = malloc(buffer_size_uv_plane);
  if (!img->planes[1]) {
    free(img->planes[0]);
    free(img);
    return NULL;
  }

  // allocating plane 2 as NULL.
  img->planes[2] = NULL;

  memset(img->planes[0], 0, buffer_size_Y_plane);  // set 0 as initial value
  memset(img->planes[1], 0, buffer_size_uv_plane); // set 0 as initial value
  return img;
}

/* Function to create an image of width x height.
 * return NULL on failure.
 * return pointer to img_t on success.
 */
img_t *img_create(uint32_t width, uint32_t height, img_format_t format) {
  if (!width || !height)
    return NULL;
  bool is_planar = img_format_is_planar(format);
  if (is_planar) {
    img_t *img_nv12 = img_create_nv12(width, height);
    if (!img_nv12) {
      fprintf(stderr, "Unable to create NV12 image\n");
      return NULL;
    }
    return img_nv12;
  }
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
  img->num_planes = 1; // non-planar formats have only 1 plane.
  img->stride[0] =
      width * bpp; // number of bytes in a row of pixel data, excluding padding
  size_t buffer_size = img->stride[0] * height;
  img->planes[0] = malloc(buffer_size);

  // Initialize other planes to NULL and strides to 0 for safety
  img->planes[1] = NULL;
  img->planes[2] = NULL;
  img->stride[1] = 0;
  img->stride[2] = 0;

  if (!img->planes[0]) {
    free(img);
    return NULL;
  }
  memset(img->planes[0], 0, buffer_size); // set 0 as initial value
  return img;
}

/* Function to destroy image */
void img_destroy(img_t *img) {
  if (!img)
    return;
  for (int i = 0; i < img->num_planes; i++) {
    if (img->planes[i]) {
      free(img->planes[i]);
    }
  }
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
  uint8_t *row = img->planes[0] + y * img->stride[0];
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
  uint8_t *row = img->planes[0] + y * img->stride[0];
  uint8_t *pixel_data = row + x * bpp;
  switch (img->format) {
  case IMG_FMT_GRAY8:
    pixel_data[0] = p.r; // Assuming r, g, b are the same for grayscale
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
