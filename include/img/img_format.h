#ifndef IMG_FORMAT_H
#define IMG_FORMAT_H
#include <stddef.h>

/* Image pixel formats */
typedef enum {
  IMG_FMT_UNKNOWN = 0,
  IMG_FMT_GRAY8,
  IMG_FMT_RGB24,
  IMG_FMT_RGBA32
} img_format_t;

size_t img_format_bytes_per_pixel(img_format_t format);
#endif
