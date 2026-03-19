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
  int w = image->width;
  int h = image->height;

  // Sample points
  int samples[3][2] = {
      {0, 0},         // top-left
      {w / 2, h / 2}, // center
      {w - 1, h - 1}  // bottom-right
  };

  for (int i = 0; i < 3; i++) {
    int x = samples[i][0];
    int y = samples[i][1];

    pixel_t p;
    if (img_get_pixel(image, x, y, &p) != 0) {
      fprintf(stderr, "Failed to get pixel at (%d, %d)\n", x, y);
      continue;
    }
    printf("Pixel (%d,%d) -> R=%u G=%u B=%u\n", x, y, p.r, p.g, p.b);
  }

  img_t *copy = img_copy(image);
  if (!copy) {
    img_destroy(image);
    image = NULL;
    printf("Failed to copy image\n");
    return -1;
  }

  for (int i = 0; i < 3; i++) {
    int x = samples[i][0];
    int y = samples[i][1];

    pixel_t p;
    if (img_get_pixel(copy, x, y, &p) != 0) {
      fprintf(stderr, "Failed to get pixel at (%d, %d) from copy\n", x, y);
      continue;
    }
    printf("COPY: Pixel (%d,%d) -> R=%u G=%u B=%u\n", x, y, p.r, p.g, p.b);
  }
  if (img_save_bmp("output_copy.bmp", copy) != 0) {
    fprintf(stderr, "Failed to save output_copy.bmp\n");
    img_destroy(image);
    image = NULL;
    return -1;
  }

  printf("Image copied and saved to output_copy.bmp\n");

  printf("Modifying copy with a red block (50x50 pixels)...\n");
  pixel_t red = {255, 0, 0, 255};

  // Create a visible red block in the top-left area
  int block_size = 50;
  for (int y = 10; y < 10 + block_size && y < copy->height; y++) {
    for (int x = 10; x < 10 + block_size && x < copy->width; x++) {
      if (img_set_pixel(copy, x, y, red) != 0) {
        fprintf(stderr, "Failed to set pixel at (%d, %d)\n", x, y);
      }
    }
  }

  // Save the modified copy
  if (img_save_bmp("output_modified.bmp", copy) != 0) {
    fprintf(stderr, "Failed to save output_modified.bmp\n");
  } else {
    printf("Modified image saved to output_modified.bmp\n");
  }

  // Clean up resources.
  img_destroy(image);
  image = NULL;
  img_destroy(copy);
  copy = NULL;
  return 0;
}
