#ifndef IMG_PROCESSING_H
#define IMG_PROCESSING_H
#include <img/img.h>

int img_box_blur(const img_t* src, img_t* dst, int kernel_size);

/*
 * Unpacks MIPI 10-bit packed RAW data (4 pixels per 5 bytes)
 * into a standard 16-bit array (1 pixel per 2 bytes).
 */
void unpack_10bit_mipi(const uint8_t *packed_data, uint16_t *out_data,
                       uint32_t width, uint32_t height, size_t stride);
#endif
