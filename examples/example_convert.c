#include <img/img_convert.h>
#include <io/img_io.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <input.bmp>\n", argv[0]);
    return -1;
  }
  img_t *image = img_load_bmp(argv[1]);
  if (!image) {
    fprintf(stderr, "Failed to load image: %s\n", argv[1]);
    return -1;
  }
  printf("created image with %d x %d \n", image->width, image->height);

  img_t *copy = img_copy(image);
  if (!copy) {
    img_destroy(image);
    image = NULL;
    printf("Failed to copy image\n");
    return -1;
  }
  // Verify img toggle function
  if (img_toggle_rgb_bgr(copy) != 0) {
    fprintf(stderr, "Failed to toggle RGB/BGR in copy\n");
  } else {
    if (img_save_bmp("output_toggled.bmp", copy) != 0) {
      fprintf(stderr, "Failed to save output_toggled.bmp\n");
    } else {
      printf("Toggled image saved to output_toggled.bmp\n");
    }
  }

  // Verify img convert gray function
  if (img_convert_rgb_to_gray(copy) != 0) {
    fprintf(stderr, "Failed to convert RGB to grayscale in copy\n");
  } else {
    if (img_save_bmp("output_gray.bmp", copy) != 0) {
      fprintf(stderr, "Failed to save output_gray.bmp\n");
    } else {
      printf("Grayscale image saved to output_gray.bmp\n");
    }
  }

  // Verify gray bmp can be loaded back correctly
  img_t *gray_image = img_load_bmp("output_gray.bmp");
  if (!gray_image) {
    fprintf(stderr, "Failed to load output_gray.bmp\n");
  } else {
    printf("format: %s\n", gray_image->format == IMG_FMT_GRAY8 ? "GRAY8" : "unknown");
    pixel_t p;
    if (img_get_pixel(gray_image, 0, 0, &p) != 0) {
      fprintf(stderr, "Failed to get pixel at (0, 0) from loaded output_gray.bmp\n");
    } else {
      printf("Pixel (0,0) in loaded output_gray.bmp -> R=%u G=%u B=%u\n", p.r, p.g, p.b);
    }
    printf("Successfully loaded output_gray.bmp with dimensions %d x %d\n",
           gray_image->width, gray_image->height);
    img_destroy(gray_image);
    gray_image = NULL;
  }

  img_destroy(copy);
  copy = NULL;
  img_destroy(image);
  image = NULL;
  return 0;
}