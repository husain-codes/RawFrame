#ifndef IMG_H
#define IMG_H

#include "img_format.h"
#include <stddef.h>
#include <stdint.h>

/*Image structure */
typedef struct {
  uint32_t width;
  uint32_t height;
  size_t stride; // number of bytes in a row of pixel data, excluding padding
  img_format_t format;
  uint8_t *data;
} img_t;

/*Pixel structure */
typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} pixel_t;

img_t *img_create(uint32_t width, uint32_t height, img_format_t format);

void img_destroy(img_t *img);

int img_get_pixel(img_t *img, int x, int y, pixel_t *out);
int img_set_pixel(img_t *img, int x, int y, pixel_t p);
#endif
