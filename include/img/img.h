#ifndef IMG_H
#define IMG_H

#include <stdint.h>
#include <stddef.h>
#include "img_format.h"

/*Image structure */
typedef struct {
    uint32_t width;
    uint32_t height;
    size_t stride;
    img_format_t format;
    uint8_t* data;
}img_t;

img_t* img_create(uint32_t width,
		uint32_t height,
		img_format_t format);

void img_destroy(img_t* img);
#endif
