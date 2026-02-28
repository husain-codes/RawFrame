#include <img/img_format.h>

size_t img_format_bytes_per_pixel(img_format_t format) {
  switch (format) {
  case IMG_FMT_UNKNOWN:
    return 0;
  case IMG_FMT_GRAY8:
    return 1;
  case IMG_FMT_RGB24:
    return 3;
  case IMG_FMT_RGBA32:
    return 4;
  default:
    return 0;
  }
}
