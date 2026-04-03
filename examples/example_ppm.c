#include <io/img_io.h>
#include <stdio.h>

int main(char argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <ppm_file_path>\n", argv[0]);
    return -1;
  }
  img_t *img = img_load_ppm(argv[1]);
  if (!img) {
    fprintf(stderr, "Failed to load ppm file\n");
    return -1;
  }
  printf("Image loaded successfully: width=%u, height=%u, format=%d\n",
         img->width, img->height, img->format);

  // print colors of first 10 pixels
  for (int i = 0; i < 10 && i < img->width * img->height; i++) {
    int x = i % img->width;
    int y = i / img->width;
    pixel_t p;
    if (img_get_pixel(img, x, y, &p) == 0) {
      printf("Pixel (%d, %d): R=%d, G=%d, B=%d, A=%d\n", x, y, p.r, p.g, p.b,
             p.a);
    } else {
      fprintf(stderr, "Failed to get pixel at (%d, %d)\n", x, y);
    }
  }

  img_t *copy = img_copy(img);
  if (!copy) {
    fprintf(stderr, "Failed to copy image\n");
    img_destroy(img);
    return -1;
  }

  if (img_save_ppm("output_copy.ppm", copy) != 0) {
    fprintf(stderr, "Failed to save ppm file\n");
    img_destroy(img);
    img_destroy(copy);
    return -1;
  }

  // converting ppm to bmp
  if (img_save_bmp("output_from_ppm.bmp", copy) != 0) {
    fprintf(stderr, "Failed to save bmp file\n");
    img_destroy(img);
    img_destroy(copy);
    return -1;
  }

  img_destroy(img);
  img_destroy(copy);
  return 0;
}