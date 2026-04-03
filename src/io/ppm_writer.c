#include <io/img_io.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Function to save image as ppm format
 * return 0 : sucess
 * return -1 : Failure
 */
int img_save_ppm(const char *path, img_t *img) {

  if (!path) {
    fprintf(stderr, "Path is NULL\n");
    return -1;
  }

  if (!img) {
    fprintf(stderr, "Input image is NULL\n");
    return -1;
  }

  if (img->format != IMG_FMT_RGB24) {
    fprintf(stderr, "unsupported image format for PPM save\n");
    return -1;
  }

  FILE *ppm_file = fopen(path, "wb+");
  if (!ppm_file) {
    fprintf(stderr, "Unable to create %s file\n", path);
    return -1;
  }

  if (fputs("P6\n", ppm_file) == EOF) {
    fprintf(stderr, "unable to write P6\n");
    fclose(ppm_file);
    return -1;
  }

  char buffer[256];
  sprintf(buffer, "%d %d\n", img->width, img->height);
  if (fputs(buffer, ppm_file) == EOF) {
    fprintf(stderr, "unable to write width and height\n");
    fclose(ppm_file);
    return -1;
  }

  if (fputs("255\n", ppm_file) == EOF) {
    fprintf(stderr, "unable to write max color value\n");
    fclose(ppm_file);
    return -1;
  }

  size_t bytes_per_row = img->width * img_format_bytes_per_pixel(IMG_FMT_RGB24);
  uint8_t *src_row;
  for (int y = 0; y < img->height; y++) {
    src_row = img->data + y * img->stride;
    if (fwrite(src_row, 1, bytes_per_row, ppm_file) != bytes_per_row) {
      fprintf(stderr, "Unable to write data values\n");
      fclose(ppm_file);
      return -1;
    }
  }
  fclose(ppm_file);
  return 0;
}
