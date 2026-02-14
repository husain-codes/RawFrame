#ifndef IMG_H
#define IMG_H

#include <stdint.h>
/* Image pixel formats */
typedef enum {
    IMG_FMT_GRAY8,
    IMG_FMT_RGB24,
    IMG_FMT_RGBA32
}img_format_t;

/*Image structure */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    img_format_t format;
    uint8_t* data;
}img_t;


#endif
