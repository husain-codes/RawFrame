#include <img/img.h>
#include <img/img_convert.h>
#include <img/img_format.h>
#include <stdio.h>

/*1: Convert RGB image to BGR
 * This function takes an image in RGB or BGR format and toggles it.
 * Parameters:  - img: A pointer to the image structure that contains the RGB or
 * BGR image data. The function modifies this image in place. Returns:  - 0 on
 * success (If the input image is toggled), -1 on failure (e.g., if the input
 * image is null or img isn't RGB or BGR).
 */
int img_toggle_rgb_bgr(img_t *img) {
  if (!img) {
    fprintf(stderr, "Image is null\n");
    return -1;
  }
  if (img->format != IMG_FMT_BGR24 && img->format != IMG_FMT_RGB24) {
    fprintf(stderr, "Image format is not RGB or BGR\n");
    return -1;
  }

  uint8_t *pixel;
  uint8_t *row_offset;
  uint8_t temp = 0;
  for (int y = 0; y < img->height; y++) {
    row_offset = img->data + y * img->stride;
    for (int x = 0; x < img->width; x++) {
      pixel = row_offset + x * 3;
      temp = pixel[0];     // getting red value into temp var
      pixel[0] = pixel[2]; // assign blue value to first pixel
      pixel[2] = temp;     // assign red value to last pixel
    }
  }

  if (img->format == IMG_FMT_BGR24) {
    img->format = IMG_FMT_RGB24;
  } else {
    img->format = IMG_FMT_BGR24;
  }

  return 0;
}
