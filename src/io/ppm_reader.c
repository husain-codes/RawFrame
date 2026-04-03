#include <io/img_io.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Function to load image from ppm format
 * return img_t * : pointer to loaded image
 * return NULL : Failure
 */
img_t *img_load_ppm(char *path) {
  FILE *ppm_file = fopen(path, "rb");
  if (!ppm_file) {
    fprintf(stderr, "fopen %s \n", path);
    return NULL;
  }
  char buffer[256];
  if (!fgets(buffer, sizeof(buffer), ppm_file)) {
    fprintf(stderr, "unable to read %s \n", path);
    fclose(ppm_file);
    return NULL;
  }
  if (strncmp(buffer, "P6", 2) != 0) {
    fprintf(stderr, "format is not P6\n");
    fclose(ppm_file);
    return NULL;
  }
  int ch;
  unsigned char comment = '#';
  while ((ch = fgetc(ppm_file)) == comment) {
    fgets(buffer, sizeof(buffer), ppm_file);
  }

  if (ungetc(ch, ppm_file) == EOF) {
    fprintf(stderr, "unable to unget char\n");
    fclose(ppm_file);
    return NULL;
  }
  uint32_t width, height, max_color;
  if (!fgets(buffer, sizeof(buffer), ppm_file)) {
    fprintf(stderr, "unable to width and height \n");
    fclose(ppm_file);
    return NULL;
  }
  sscanf(buffer, "%d%d", &width, &height);
  if (!fgets(buffer, sizeof(buffer), ppm_file)) {
    fprintf(stderr, "unable to get max color value\n");
    fclose(ppm_file);
    return NULL;
  };
  sscanf(buffer, "%d", &max_color);

  if (max_color != 255) {
    fprintf(stderr, "unsupported max color value : %d\n", max_color);
    fclose(ppm_file);
    return NULL;
  }

  img_t *ppm_img = img_create(width, height, IMG_FMT_RGB24);
  if (!ppm_img) {
    fprintf(stderr, "Unable to create ppm image\n");
    fclose(ppm_file);
    return NULL;
  }

  uint8_t *dst_row;
  int bytes_per_row = width * img_format_bytes_per_pixel(IMG_FMT_RGB24);
  for (int y = 0; y < height; y++) {
    dst_row = ppm_img->data + y * ppm_img->stride;
    if (fread(dst_row, 1, bytes_per_row, ppm_file) != bytes_per_row) {
      fprintf(stderr, "Unable to copy raw pixel values at height : %d\n", y);
      fclose(ppm_file);
      img_destroy(ppm_img);
      return NULL;
    }
  }
  fclose(ppm_file);
  return ppm_img;
}
