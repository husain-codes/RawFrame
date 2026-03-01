#include <io/img_io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

img_t *img_copy(img_t *img) {
  if (!img) {
    fprintf(stderr, "Input image is NULL\n");
    return NULL;
  }

  img_t *copy = img_create(img->width, img->height, img->format);
  if (!copy) {
    fprintf(stderr, "Unable to create copy of image\n");
    return NULL;
  }

  memcpy(copy->data, img->data, img->height * img->stride);
  return copy;
}