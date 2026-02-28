#ifndef IMG_IO_H
#define IMG_IO_H
#include <img/img.h>
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
  uint16_t signature;
  uint32_t file_size;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t data_offset;
} BMHEADER;

typedef struct {
  uint32_t info_header_size;
  int32_t width;
  int32_t height;
  uint16_t planes;
  uint16_t bits_per_pixel;
  uint32_t compression;
  uint32_t image_size;
  uint32_t x_pixels_per_m;
  uint32_t y_pixels_per_m;
  uint32_t color_used;
  uint32_t imp_colors;
} BMINFOHEADER;
#pragma pack(pop)

img_t *img_load_bmp(char *path);
img_t *img_copy(img_t *img);
#endif
