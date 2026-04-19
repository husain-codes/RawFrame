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

  memcpy(copy->planes[0], img->planes[0],
         img_format_plane_height(img->format, img->height, 0) * img->stride[0]);

  if (img_format_is_planar(img->format)) {
    for (int i = 1; i < img->num_planes; i++) {
      if (img->planes[i]) {
        memcpy(copy->planes[i], img->planes[i],
               img_format_plane_height(img->format, img->height, i) *
                   img->stride[i]);
      }
    }
  }
  return copy;
}