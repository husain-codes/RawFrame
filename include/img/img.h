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

img_t *img_create(uint32_t width, uint32_t height, img_format_t format);

void img_destroy(img_t *img);
#endif
