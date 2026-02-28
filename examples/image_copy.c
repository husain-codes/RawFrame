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

    unsigned char *pixel = image->data + (y * w + x) * 3;
    unsigned char r = pixel[0];
    unsigned char g = pixel[1];
    unsigned char b = pixel[2];
    printf("Pixel (%d,%d) -> R=%u G=%u B=%u\n", x, y, r, g, b);
  }

  img_t *copy = img_copy(image);
  if (!copy) {
    img_destroy(image);
    image = NULL;
    printf("Failed to copy image\n");
    return -1;
  }
  int wc = copy->width;
  int hc = copy->height;
  for (int i = 0; i < 3; i++) {
    int x = samples[i][0];
    int y = samples[i][1];

    unsigned char *pixel = copy->data + (y * wc + x) * 3;
    unsigned char r = pixel[0];
    unsigned char g = pixel[1];
    unsigned char b = pixel[2];
    printf("COPY: Pixel (%d,%d) -> R=%u G=%u B=%u\n", x, y, r, g, b);
  }
  /*     if (img_save_bmp("output.bmp",copy) != 0)
       {
           fprintf(stderr,"Failed to save output.bmp\n");
       }
 */
  printf("Image copied and saved to output.bmp\n");
  // Clean up resources.
  img_destroy(image);
  image = NULL;
  // img_destroy(copy);
  // copy = NULL;
  return 0;
}
