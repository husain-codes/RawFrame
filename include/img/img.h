#ifndef IMG_H
#define IMG_H

#include <stdint.h>
#include "img_format.h"

/*Image structure */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    img_format_t format;
    uint8_t* data;
}img_t;


#endif
