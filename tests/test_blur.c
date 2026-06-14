#include <raw_frame.h>
#include <unity.h>

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void test_img_box_blur_null_pointers() {
  img_t *src = img_create(4, 4, IMG_FMT_GRAY8);
  img_t *dst = img_create(4, 4, IMG_FMT_GRAY8);

  // Test if either pointer is NULL
  TEST_ASSERT_EQUAL_INT(-1, img_box_blur(NULL, dst, 3));
  TEST_ASSERT_EQUAL_INT(-1, img_box_blur(src, NULL, 3));

  img_destroy(src);
  img_destroy(dst);
}

void test_img_box_blur_mismatched_attributes() {
  img_t *src = img_create(4, 4, IMG_FMT_GRAY8);
  img_t *dst_wrong_dim = img_create(6, 4, IMG_FMT_GRAY8);
  img_t *dst_wrong_fmt = img_create(4, 4, IMG_FMT_RGB24);

  // Dimensions must match
  TEST_ASSERT_EQUAL_INT(-1, img_box_blur(src, dst_wrong_dim, 3));
  // Formats must match
  TEST_ASSERT_EQUAL_INT(-1, img_box_blur(src, dst_wrong_fmt, 3));

  img_destroy(src);
  img_destroy(dst_wrong_dim);
  img_destroy(dst_wrong_fmt);
}

void test_img_box_blur_invalid_kernel() {
  img_t *src = img_create(4, 4, IMG_FMT_GRAY8);
  img_t *dst = img_create(4, 4, IMG_FMT_GRAY8);

  // Kernel must be odd and >= 3
  TEST_ASSERT_EQUAL_INT(-1, img_box_blur(src, dst, 1)); // Too small
  TEST_ASSERT_EQUAL_INT(-1, img_box_blur(src, dst, 2)); // Even number
  TEST_ASSERT_EQUAL_INT(-1, img_box_blur(src, dst, 4)); // Even number

  img_destroy(src);
  img_destroy(dst);
}

void test_img_box_blur_grayscale_success() {
  // Create a 3x3 image (perfect fit for a kernel size of 3)
  img_t *src = img_create(3, 3, IMG_FMT_GRAY8);
  img_t *dst = img_create(3, 3, IMG_FMT_GRAY8);
  TEST_ASSERT_NOT_NULL(src);
  TEST_ASSERT_NOT_NULL(dst);

  /*
   * Setup a known pixel grid on plane 0:
   * [ 10,  20,  30 ]
   * [ 40,  50,  60 ]
   * [ 70,  80,  90 ]
   * Total Sum = 450. Kernel Area = 9.
   * Center pixel average value should be exactly 450 / 9 = 50.
   */
  uint8_t *src_data = src->planes[0];
  src_data[0] = 10;
  src_data[1] = 20;
  src_data[2] = 30;
  src_data[3] = 40;
  src_data[4] = 50;
  src_data[5] = 60;
  src_data[6] = 70;
  src_data[7] = 80;
  src_data[8] = 90;

  // Run box blur with a kernel size of 3 (radius = 1)
  int result = img_box_blur(src, dst, 3);
  TEST_ASSERT_EQUAL_INT(0, result);

  uint8_t *dst_data = dst->planes[0];

  // The center pixel (1, 1) has index = (1 * stride) + 1 = 4
  // It spans the whole matrix, accumulating a perfect average of 50
  TEST_ASSERT_EQUAL_UINT8(50, dst_data[4]);

  // Check edge padding consistency
  // (Your current initialization clones the starting sliding sums across
  // columns/rows)
  TEST_ASSERT_NOT_EQUAL(0, dst_data[0]);

  img_destroy(src);
  img_destroy(dst);
}

void test_img_box_blur_rgb24_success() {
  // Create a 3x3 RGB24 image
  img_t *src = img_create(3, 3, IMG_FMT_RGB24);
  img_t *dst = img_create(3, 3, IMG_FMT_RGB24);
  TEST_ASSERT_NOT_NULL(src);
  TEST_ASSERT_NOT_NULL(dst);

  // Fill plane 0 with a solid color grid (e.g., pure Red pixels)
  // Each row has 3 pixels * 3 channels = 9 bytes.
  // Total elements = 27 bytes.
  uint8_t *src_data = src->planes[0];
  for (int i = 0; i < 3 * 3 * 3; i += 3) {
    src_data[i + 0] = 90; // Red
    src_data[i + 1] = 0;  // Green
    src_data[i + 2] = 0;  // Blue
  }

  // Run box blur with a kernel size of 3
  int result = img_box_blur(src, dst, 3);
  TEST_ASSERT_EQUAL_INT(0, result);

  // The center pixel should evaluate to exactly 90 for the Red channel
  uint8_t *dst_data = dst->planes[0];
  int center_idx = (1 * dst->stride[0]) + (1 * 3);       // Row 1, Pixel 1
  TEST_ASSERT_EQUAL_UINT8(90, dst_data[center_idx + 0]); // Red
  TEST_ASSERT_EQUAL_UINT8(0, dst_data[center_idx + 1]);  // Green
  TEST_ASSERT_EQUAL_UINT8(0, dst_data[center_idx + 2]);  // Blue

  img_destroy(src);
  img_destroy(dst);
}

void test_img_box_blur_yuv420p_pipeline() {
  pixel_t check_pixel;

  // 1. Setup a clean 4x4 RGB24 source image (Needs to be even for YUV420P)
  img_t *rgb_src = img_create(4, 4, IMG_FMT_RGB24);
  TEST_ASSERT_NOT_NULL(rgb_src);

  // Fill with a uniform mid-gray color (R=128, G=128, B=128)
  uint8_t *rgb_data = rgb_src->planes[0];
  for (int i = 0; i < 4 * 4 * 3; i++) {
    rgb_data[i] = 128;
  }

  // Print Center Pixel directly after initializing raw memory buffer
  img_get_pixel(rgb_src, 1, 1, &check_pixel);
  printf("\n[DEBUG] 1. Initial RGB Source Center (1,1): R=%d, G=%d, B=%d\n",
         check_pixel.r, check_pixel.g, check_pixel.b);

  // 2. Convert RGB -> YUV420P
  img_t *yuv_src = img_convert_rgb_to_yuv420p(rgb_src);
  TEST_ASSERT_NOT_NULL(yuv_src);

  // Directly print the raw Y, U, V components from their independent plane
  // coordinates at (1,1) For YUV420P: Chroma dimensions are halved (1/2 = index
  // 0)
  uint8_t y_val = yuv_src->planes[0][1 * yuv_src->stride[0] + 1];
  uint8_t u_val = yuv_src->planes[1][0 * yuv_src->stride[1] + 0];
  uint8_t v_val = yuv_src->planes[2][0 * yuv_src->stride[2] + 0];
  printf("[DEBUG] 2. Converted YUV Source Center (1,1): Y=%d, U=%d, V=%d\n",
         y_val, u_val, v_val);

  img_t *yuv_dst = img_create(4, 4, IMG_FMT_YUV420P);
  TEST_ASSERT_NOT_NULL(yuv_dst);

  // 3. Blur directly inside the subsampled YUV format planes
  int blur_res = img_box_blur(yuv_src, yuv_dst, 3);
  TEST_ASSERT_EQUAL_INT(0, blur_res);

  // Print the raw Y, U, V values after the blur filter pass
  uint8_t y_blur = yuv_dst->planes[0][1 * yuv_dst->stride[0] + 1];
  uint8_t u_blur = yuv_dst->planes[1][0 * yuv_dst->stride[1] + 0];
  uint8_t v_blur = yuv_dst->planes[2][0 * yuv_dst->stride[2] + 0];
  printf("[DEBUG] 3. Post-Blur YUV Target Center (1,1) : Y=%d, U=%d, V=%d\n",
         y_blur, u_blur, v_blur);

  // 4. Convert back to RGB to verify human-readable values
  img_t *rgb_dst = img_convert_yuv420p_to_rgb(yuv_dst);
  TEST_ASSERT_NOT_NULL(rgb_dst);

  // Verify dimensions stayed matching and intact
  TEST_ASSERT_EQUAL(rgb_src->width, rgb_dst->width);
  TEST_ASSERT_EQUAL(rgb_src->height, rgb_dst->height);

  // Get final center pixel values to check color fidelity after the blur pass
  img_get_pixel(rgb_dst, 1, 1, &check_pixel);
  printf("[DEBUG] 4. Final Reconverted RGB Center (1,1): R=%d, G=%d, B=%d\n",
         check_pixel.r, check_pixel.g, check_pixel.b);

  TEST_ASSERT_INT_WITHIN(10, 128, check_pixel.r);
  TEST_ASSERT_INT_WITHIN(10, 128, check_pixel.g);
  TEST_ASSERT_INT_WITHIN(10, 128, check_pixel.b);

  img_destroy(rgb_src);
  img_destroy(yuv_src);
  img_destroy(yuv_dst);
  img_destroy(rgb_dst);
}

void test_img_box_blur_nv12_pipeline() {
  // 1. Setup a clean 4x4 RGB24 source image
  img_t *rgb_src = img_create(4, 4, IMG_FMT_RGB24);
  TEST_ASSERT_NOT_NULL(rgb_src);

  uint8_t *rgb_data = rgb_src->planes[0];
  for (int i = 0; i < 4 * 4 * 3; i++) {
    rgb_data[i] = 100; // Uniform color profile
  }

  // 2. Convert RGB -> NV12
  img_t *nv12_src = img_convert_rgb_to_nv12(
      rgb_src); // Adjust to your conversion function name
  TEST_ASSERT_NOT_NULL(nv12_src);

  img_t *nv12_dst = img_create(4, 4, IMG_FMT_NV12);
  TEST_ASSERT_NOT_NULL(nv12_dst);

  // 3. Run the blur pass (Validates your interleaved NV12 stride code!)
  int blur_res = img_box_blur(nv12_src, nv12_dst, 3);
  TEST_ASSERT_EQUAL_INT(0, blur_res);

  // 4. Convert back NV12 -> RGB
  img_t *rgb_dst = img_convert_nv12_to_rgb(nv12_dst);
  TEST_ASSERT_NOT_NULL(rgb_dst);

  pixel_t out_pixel;
  img_get_pixel(rgb_dst, 1, 1, &out_pixel);
  TEST_ASSERT_INT_WITHIN(10, 100, out_pixel.r);
  TEST_ASSERT_INT_WITHIN(10, 100, out_pixel.g);
  TEST_ASSERT_INT_WITHIN(10, 100, out_pixel.b);

  img_destroy(rgb_src);
  img_destroy(nv12_src);
  img_destroy(nv12_dst);
  img_destroy(rgb_dst);
}

#include <time.h>

void test_img_box_blur_performance_comparison() {
  // Use a large image size (e.g., Full HD 1920x1080) to get measurable time
  // differences
  int bench_w = 1920;
  int bench_h = 1080;
  int k_size = 5; // Use a larger kernel size to increase computational workload

  printf("\n--- Starting Blur Performance Benchmark (%dx%d, Kernel: %d) ---\n",
         bench_w, bench_h, k_size);

  // 1. Setup RGB Images
  img_t *rgb_src = img_create(bench_w, bench_h, IMG_FMT_RGB24);
  img_t *rgb_dst = img_create(bench_w, bench_h, IMG_FMT_RGB24);
  TEST_ASSERT_NOT_NULL(rgb_src);
  TEST_ASSERT_NOT_NULL(rgb_dst);

  // 2. Setup YUV420P Images
  img_t *yuv_src = img_create(bench_w, bench_h, IMG_FMT_YUV420P);
  img_t *yuv_dst = img_create(bench_w, bench_h, IMG_FMT_YUV420P);
  TEST_ASSERT_NOT_NULL(yuv_src);
  TEST_ASSERT_NOT_NULL(yuv_dst);

  // --- Benchmark RGB24 Blur ---
  clock_t start_rgb = clock();
  int res_rgb = img_box_blur(rgb_src, rgb_dst, k_size);
  clock_t end_rgb = clock();
  double time_rgb = (double)(end_rgb - start_rgb) / CLOCKS_PER_SEC;

  TEST_ASSERT_EQUAL_INT(0, res_rgb);
  printf("RGB24 Blur Processing Time  : %.4f seconds\n", time_rgb);

  // --- Benchmark YUV420P Blur ---
  clock_t start_yuv = clock();
  int res_yuv = img_box_blur(yuv_src, yuv_dst, k_size);
  clock_t end_yuv = clock();
  double time_yuv = (double)(end_yuv - start_yuv) / CLOCKS_PER_SEC;

  TEST_ASSERT_EQUAL_INT(0, res_yuv);
  printf("YUV420P Blur Processing Time: %.4f seconds\n", time_yuv);

  // Calculate and display optimization metrics
  double speedup = (time_rgb / time_yuv);
  printf("YUV420P is processing at %.2fx the speed of RGB24\n", speedup);
  printf("-----------------------------------------------------------------\n");

  // Cleanup all allocated benchmark frames
  img_destroy(rgb_src);
  img_destroy(rgb_dst);
  img_destroy(yuv_src);
  img_destroy(yuv_dst);
}

/*
 * Note: The above test cases are designed to validate the core functionality of
 * the box blur implementation across different formats and edge cases. The
 * grayscale and RGB tests confirm that the blur produces expected average
 * values in a controlled environment. The YUV420P and NV12 pipeline tests
 * ensure that the blur can be applied directly to subsampled formats without
 * crashing and that the color fidelity is maintained after converting back to
 * RGB. You can expand these tests further by adding more complex patterns,
 * larger images, and varying kernel sizes to stress test the implementation.
 */
int main() {
  UNITY_BEGIN();
  RUN_TEST(test_img_box_blur_null_pointers);
  RUN_TEST(test_img_box_blur_mismatched_attributes);
  RUN_TEST(test_img_box_blur_invalid_kernel);
  RUN_TEST(test_img_box_blur_grayscale_success);
  RUN_TEST(test_img_box_blur_rgb24_success);
  RUN_TEST(test_img_box_blur_yuv420p_pipeline);
  RUN_TEST(test_img_box_blur_nv12_pipeline);
  // Run the Performance Metrics Benchmark Pass
  RUN_TEST(test_img_box_blur_performance_comparison);
  return UNITY_END();
}
