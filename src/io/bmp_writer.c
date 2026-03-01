#include <io/img_io.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Function to initiliaze bmp headers
 * return 0 : sucess
 * return -1 : Failure
 */
int initialize_bmp_header(BMHEADER *header, BMINFOHEADER *info_header,
                          img_t *img) {
  if (!img) {
    fprintf(stderr, "Input image is NULL\n");
    return -1;
  }

  // Initialize BMP Header
  header->signature = 0x4D42;
  header->reserved1 = 0;
  header->reserved2 = 0;
  header->data_offset = sizeof(BMHEADER) + sizeof(BMINFOHEADER);

  /* BMP rows are padded to a multiple of four bytes.  Compute the
     padded size so that we can fill in the file_size and image_size
     fields correctly.  The info_header.height will be positive to
     indicate a bottom‑up bitmap (the usual layout). */
  info_header->info_header_size = 40;
  info_header->width = img->width;
  info_header->height = img->height; /* positive -> bottom‑up */
  info_header->planes = 1;
  info_header->bits_per_pixel = img_format_bytes_per_pixel(img->format) * 8;
  info_header->compression = 0; /* no compression */

  int bytes_per_row = img->width * (info_header->bits_per_pixel / 8);
  int padded_row = (bytes_per_row + 3) & ~3; /* align up to 4 */

  header->file_size = header->data_offset + padded_row * img->height;
  info_header->image_size = padded_row * img->height;
  info_header->x_pixels_per_m = 0;
  info_header->y_pixels_per_m = 0;
  info_header->color_used = 0;
  info_header->imp_colors = 0;
  return 0;
}

/*
 * Function to save image as bmp format
 * return 0 : sucess
 * return -1 : Failure
 */
int img_save_bmp(const char *path, img_t *img) {
  if (!img) {
    fprintf(stderr, "Input image is NULL\n");
    return -1;
  }

  FILE *bmp_file = fopen(path, "wb+");
  if (!bmp_file) {
    fprintf(stderr, "Unable to create %s file\n", path);
    return -1;
  }

  BMHEADER header;
  BMINFOHEADER info_header;
  if (initialize_bmp_header(&header, &info_header, img) != 0) {
    fprintf(stderr, "Unable to initialize headers\n");
    fclose(bmp_file);
    return -1;
  }

  if (!fwrite(&header, sizeof(BMHEADER), 1, bmp_file)) {
    fprintf(stderr, "Unable to populate header values to bm_file\n");
    fclose(bmp_file);
    return -1;
  }

  if (!fwrite(&info_header, sizeof(BMINFOHEADER), 1, bmp_file)) {
    fprintf(stderr, "Unable to populate header values to bm_file\n");
    fclose(bmp_file);
    return -1;
  }
  int dst_y = 0;

  int bytes_per_row = img->width * img_format_bytes_per_pixel(img->format);
  int padded_row = (bytes_per_row + 3) & ~3; /* align up to 4 */
  int padding = padded_row - bytes_per_row;
  unsigned char padding_data[3] = {0, 0, 0}; /* up to 3 bytes of padding */

  int height = abs(info_header.height);
  for (int y = 0; y < height; y++) {
    dst_y = height - 1 - y;
    unsigned char *dst_row = img->data + dst_y * img->stride;
    if (!fwrite(dst_row, bytes_per_row, 1, bmp_file)) {
      fprintf(stderr, "Unable to write data values\n");
      fclose(bmp_file);
      return -1;
    }
    if (padding && !fwrite(padding_data, padding, 1, bmp_file)) {
      fprintf(stderr, "Unable to write padding values\n");
      fclose(bmp_file);
      return -1;
    }
  }
  fclose(bmp_file);
  return 0;
}
