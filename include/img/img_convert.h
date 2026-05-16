#ifndef IMG_CONVERT_H
#define IMG_CONVERT_H
#include <img/img.h>

int img_toggle_rgb_bgr(img_t *img); // inplace conversion
int img_convert_rgb_to_gray(
    img_t *img); // inplace conversion, lossy conversion, original RGB data will
                 // be discarded, irreversible operation
int img_convert_rgb_to_yuv444(
    img_t *img); // inplace conversion, lossy conversion, original RGB data will
                 // be discarded, irreversible operation. Note: Function expects
                 // the input image to be in RGB format. If the image is in BGR
                 // format, it should be toggled to RGB first using
                 // img_toggle_rgb_bgr before calling this function.
int img_convert_yuv444_to_rgb(img_t *img); // inplace conversion
img_t *img_convert_rgb_to_nv12(const img_t *img);
img_t *img_convert_nv12_to_rgb(const img_t *img);
img_t *img_convert_rgb_to_yuv420p(const img_t *img);
img_t *img_convert_yuv420p_to_rgb(const img_t *img);
#endif
