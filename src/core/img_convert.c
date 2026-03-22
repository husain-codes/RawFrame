#include <img/img.h>
#include <img/img_convert.h>
#include <img/img_format.h>
#include <stdio.h>
#include <stdlib.h>

/* Convert RGB image to BGR
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

/* Convert RGB image to Grayscale
 * This function converts an RGB image to grayscale using the luminosity method.
 * Parameters:  - img: A pointer to the image structure that contains the RGB
 * image data. The function modifies this image in place, changing its format to
 * grayscale. it is lossy conversion, so the original RGB data will be
 * discarded. this a irreversible operation. Once converted to grayscale, the
 * image cannot be converted back to RGB with the original colors. Returns:  - 0
 * on success (If the input RGB image is successfully converted to grayscale),
 * -1 on failure (e.g., if the input image is null or not in RGB format). Note:
 * Function expects the input image to be in RGB format. If the image is in BGR
 * format, it should be toggled to RGB first using img_toggle_rgb_bgr before
 * calling this function.
 */
int img_convert_rgb_to_gray(img_t *img) {
  if (!img) {
    fprintf(stderr, "Image is null\n");
    return -1;
  }

  if (img->format != IMG_FMT_RGB24) {
    fprintf(stderr, "Image format is not RGB\n");
    return -1;
  }
  size_t buffer_size = img->width * img->height;
  uint8_t *new_data = (uint8_t *)malloc(buffer_size);
  if (!new_data) {
    fprintf(stderr, "Unable to allocate memory of size %zu\n", buffer_size);
    return -1;
  }

  uint8_t *p;      // pointer to the current pixel in the original RGB image
  uint8_t *gray_p; // pointer to the current pixel in the new grayscale image
  uint8_t *row_offset;      // pointer to the start of the current row in the
                            // original RGB image
  uint8_t *row_offset_gray; // pointer to the start of the current row in the
                            // new grayscale image
  size_t bpp = img_format_bytes_per_pixel(img->format);
  for (int y = 0; y < img->height; y++) {
    row_offset = img->data + y * img->stride;
    row_offset_gray = new_data + y * img->width;
    for (int x = 0; x < img->width; x++) {
      p = row_offset + x * bpp;
      gray_p = row_offset_gray + x;
      gray_p[0] = (uint8_t)(0.299 * p[0] + 0.587 * p[1] + 0.114 * p[2]);
    }
  }

  free(img->data);
  img->data = new_data;
  img->format = IMG_FMT_GRAY8;
  img->stride = img->width;
  return 0;
}

/* Convert RGB image to YUV444
 * This function converts an RGB image to YUV444 format using the standard
 * conversion formula. Parameters:  - img: A pointer to the image structure that
 * contains the RGB image data. The function modifies this image in place,
 * changing its format to YUV444. Returns:  - 0 on success (If the input RGB
 * image is successfully converted to YUV444), -1 on failure (e.g., if the input
 * image is null or not in RGB format). Note: Function expects the input image
 * to be in RGB format. If the image is in BGR format, it should be toggled to
 * RGB first using img_toggle_rgb_bgr before calling this function.
 */
int img_convert_rgb_to_yuv444(img_t *img) {
  if (!img) {
    fprintf(stderr, "Image is null \n");
    return -1;
  }

  if (img->format != IMG_FMT_RGB24) {
    fprintf(stderr, "Image format is not RGB\n");
    return -1;
  }

  uint8_t *pixel;
  uint8_t *row_offset;
  uint8_t Y, U, V;
  size_t bpp = img_format_bytes_per_pixel(img->format);
  for (int y = 0; y < img->height; y++) {
    row_offset = img->data + y * img->stride;
    for (int x = 0; x < img->width; x++) {
      pixel = row_offset + x * bpp;
      Y = (uint8_t)(0.299 * pixel[0] + 0.587 * pixel[1] + 0.114 * pixel[2]);
      U = (uint8_t)(-0.169 * pixel[0] - 0.331 * pixel[1] + 0.5 * pixel[2] +
                    128);
      V = (uint8_t)(0.5 * pixel[0] - 0.419 * pixel[1] - 0.081 * pixel[2] + 128);
      pixel[0] = Y;
      pixel[1] = U;
      pixel[2] = V;
    }
  }
  img->format = IMG_FMT_YUV444;
  return 0;
}

/* Convert YUV444 image to RGB
 * This function converts a YUV444 image back to RGB format using the standard
 * conversion formula. Parameters:  - img: A pointer to the image structure that
 * contains the YUV444 image data. The function modifies this image in place,
 * changing its format to RGB. Returns:  - 0 on success (If the input YUV444
 * image is successfully converted to RGB), -1 on failure (e.g., if the input
 * image is null or not in YUV444 format).
 */
int img_convert_yuv444_to_rgb(img_t *img) {
  if (!img) {
    fprintf(stderr, "Image is null \n");
    return -1;
  }

  if (img->format != IMG_FMT_YUV444) {
    fprintf(stderr, "Image format is not RGB\n");
    return -1;
  }

  uint8_t *pixel;
  uint8_t *row_offset;
  uint8_t R, G, B;
  double r_val, g_val, b_val;
  size_t bpp = img_format_bytes_per_pixel(img->format);
  for (int y = 0; y < img->height; y++) {
    row_offset = img->data + y * img->stride;
    for (int x = 0; x < img->width; x++) {
      pixel = row_offset + x * bpp;
      r_val = pixel[0] + 1.402 * (pixel[2] - 128); // Y + 1.402 * (V - 128)
      g_val =
          pixel[0] - 0.344 * (pixel[1] - 128) -
          0.714 * (pixel[2] - 128); // Y - 0.344 * (U - 128) - 0.714 * (V - 128)
      b_val = pixel[0] + 1.772 * (pixel[1] - 128); // Y + 1.772 * (U - 128)
      R = r_val < 0 ? 0 : (r_val > 255 ? 255 : (uint8_t)r_val);
      G = g_val < 0 ? 0 : (g_val > 255 ? 255 : (uint8_t)g_val);
      B = b_val < 0 ? 0 : (b_val > 255 ? 255 : (uint8_t)b_val);
      pixel[0] = R;
      pixel[1] = G;
      pixel[2] = B;
    }
  }

  img->format = IMG_FMT_RGB24;
  return 0;
}
