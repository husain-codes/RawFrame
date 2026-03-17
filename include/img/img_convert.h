#ifndef IMG_CONVERT_H
#define IMG_CONVERT_H
#include <img/img.h>

int img_toggle_rgb_bgr(img_t *img);
int img_toggle_rgb_bgr_copy(img_t *img_in, img_t *img_out);
#endif