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
    printf("format: %s\n",
           gray_image->format == IMG_FMT_GRAY8 ? "GRAY8" : "unknown");
    pixel_t p;
    if (img_get_pixel(gray_image, 0, 0, &p) != 0) {
      fprintf(stderr,
              "Failed to get pixel at (0, 0) from loaded output_gray.bmp\n");
    } else {
      printf("Pixel (0,0) in loaded output_gray.bmp -> R=%u G=%u B=%u\n", p.r,
             p.g, p.b);
    }
    printf("Successfully loaded output_gray.bmp with dimensions %d x %d\n",
           gray_image->width, gray_image->height);
    img_destroy(gray_image);
    gray_image = NULL;
  }

  // verify img convert rgb to nv12 function
  if (img_toggle_rgb_bgr(image) != 0) {
    fprintf(stderr, "Failed to toggle RGB/BGR in original image\n");
  } else {
    printf("Toggled original image from BGR to RGB format\n");
  }

  img_t *nv12_image = img_convert_rgb_to_nv12(image);
  if (!nv12_image) {
    fprintf(stderr, "Failed to convert RGB to NV12\n");
  } else {
    printf("Successfully converted RGB to NV12 with dimensions %d x %d\n",
           nv12_image->width, nv12_image->height);
    img_destroy(nv12_image);
    nv12_image = NULL;
  }

  //verify round trip conversion from RGB to NV12 and back to RGB
  img_t *nv12_image_round_trip = img_convert_rgb_to_nv12(image);
  //print yuv values at center pixel
  if (!nv12_image_round_trip) {
    fprintf(stderr, "Failed to convert RGB to NV12 for round trip test\n");
  } else {
    uint8_t *y_plane = nv12_image_round_trip->planes[0];
    uint8_t *uv_plane = nv12_image_round_trip->planes[1];
    int center_index = (nv12_image_round_trip->height / 2) * nv12_image_round_trip->stride[0] + (nv12_image_round_trip->width / 2);
    uint8_t Y = y_plane[center_index];
    int center_uv_index = (nv12_image_round_trip->height / 4) * (nv12_image_round_trip->stride[1]) + (nv12_image_round_trip->width / 2);
    uint8_t U = uv_plane[center_uv_index ]; // U value for the center pixel
    uint8_t V = uv_plane[center_uv_index + 1];
    printf("Center pixel in NV12: Y=%u U=%u V=%u\n", Y, U, V);
    printf("Successfully converted RGB to NV12 for round trip test with dimensions %d x %d\n",
           nv12_image_round_trip->width, nv12_image_round_trip->height);
  }

  img_t *rgb_image_round_trip = img_convert_nv12_to_rgb(nv12_image_round_trip);
  if (!rgb_image_round_trip) {
    fprintf(stderr, "Failed to convert NV12 back to RGB\n");
  } else {
    img_toggle_rgb_bgr(rgb_image_round_trip); // Toggle back to BGR for correct color display in BMP
    img_save_bmp("output_nv12_to_rgb.bmp", rgb_image_round_trip);
    printf("Successfully converted NV12 back to RGB with dimensions %d x %d\n",
           rgb_image_round_trip->width, rgb_image_round_trip->height);
    //print rgb at center pixel
    pixel_t center_pixel;
    img_get_pixel(rgb_image_round_trip, rgb_image_round_trip->width / 2, rgb_image_round_trip->height / 2, &center_pixel);
    printf("Center pixel in round trip NV12->RGB: R=%u G=%u B=%u\n", center_pixel.r, center_pixel.g, center_pixel.b);
    img_destroy(rgb_image_round_trip);
    rgb_image_round_trip = NULL;
  }

  //Verify round trip conversion from RGB to YUV420P and back to RGB
  img_t *yuv420p_image_round_trip = img_convert_rgb_to_yuv420p(image);
  //print yuv values at center pixel
  if (!yuv420p_image_round_trip) {
    fprintf(stderr, "Failed to convert RGB to YUV420P for round trip test\n");
  } else {
    uint8_t *y_plane = yuv420p_image_round_trip->planes[0];
    uint8_t *u_plane = yuv420p_image_round_trip->planes[1];
    uint8_t *v_plane = yuv420p_image_round_trip->planes[2];
    int center_index = (yuv420p_image_round_trip->height / 2) * yuv420p_image_round_trip->stride[0] + (yuv420p_image_round_trip->width / 2);
    uint8_t Y = y_plane[center_index];
    uint8_t U = u_plane[(yuv420p_image_round_trip->height / 2) * (yuv420p_image_round_trip->stride[1]) + (yuv420p_image_round_trip->width / 2)];
    uint8_t V = v_plane[(yuv420p_image_round_trip->height / 2) * (yuv420p_image_round_trip->stride[2]) + (yuv420p_image_round_trip->width / 2)];
    printf("Center pixel in YUV420P: Y=%u U=%u V=%u\n", Y, U, V);
    printf("Successfully converted RGB to YUV420P for round trip test with dimensions %d x %d\n",
           yuv420p_image_round_trip->width, yuv420p_image_round_trip->height);
  }
  img_t *rgb_image_round_trip_yuv = img_convert_yuv420p_to_rgb(yuv420p_image_round_trip);

  if (!rgb_image_round_trip_yuv) {
    fprintf(stderr, "Failed to convert YUV420P back to RGB\n");
  } else {
    img_toggle_rgb_bgr(rgb_image_round_trip_yuv); // Toggle back to BGR for correct color display in BMP
    img_save_bmp("output_yuv420p_to_rgb.bmp", rgb_image_round_trip_yuv);
    printf("Successfully converted YUV420P back to RGB with dimensions %d x %d\n",
           rgb_image_round_trip_yuv->width, rgb_image_round_trip_yuv->height);
    //print rgb at center pixel
    pixel_t center_pixel;
    img_get_pixel(rgb_image_round_trip_yuv, rgb_image_round_trip_yuv->width / 2, rgb_image_round_trip_yuv->height / 2, &center_pixel);
    printf("Center pixel in round trip YUV420P->RGB: R=%u G=%u B=%u\n", center_pixel.r, center_pixel.g, center_pixel.b);
    img_destroy(rgb_image_round_trip_yuv);
    rgb_image_round_trip_yuv = NULL;
  }

  img_destroy(image);
  image = NULL;
  img_destroy(yuv420p_image_round_trip);
  yuv420p_image_round_trip = NULL;
  img_destroy(nv12_image_round_trip);
  nv12_image_round_trip = NULL;
  img_destroy(copy);
  copy = NULL;
  img_destroy(image);
  image = NULL;
  return 0;
}