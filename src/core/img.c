#include <img/img.h>
#include <stddef.h>
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
