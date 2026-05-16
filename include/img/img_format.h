#ifndef IMG_FORMAT_H
#define IMG_FORMAT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Image pixel formats */
typedef enum {
  IMG_FMT_UNKNOWN = 0,
  IMG_FMT_GRAY8,
  IMG_FMT_RGB24,
  IMG_FMT_BGR24,
  IMG_FMT_RGBA32,
  IMG_FMT_YUV444,
  IMG_FMT_NV12,
  IMG_FMT_YUV420P
} img_format_t;

size_t img_format_bytes_per_pixel(img_format_t format);
bool img_format_is_planar(img_format_t format);
uint32_t img_format_plane_height(img_format_t format, uint32_t height,
                                 int plane_index);
#endif
