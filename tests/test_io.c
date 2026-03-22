#include <io/img_io.h>
#include <unity.h>

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void test_img_save_bmp_null() {
  TEST_ASSERT_EQUAL(-1, img_save_bmp("test_output.bmp", NULL));
}

void test_img_save_bmp_bgr_roundtrip() {
  img_t *image = img_create(2, 2, IMG_FMT_BGR24);
  pixel_t p1 = {255, 0, 0, 255};   // Red
  pixel_t p2 = {0, 255, 0, 255};   // Green
  pixel_t p3 = {0, 0, 255, 255};   // Blue
  pixel_t p4 = {255, 255, 0, 255}; // Yellow
  img_set_pixel(image, 0, 0, p1);
  img_set_pixel(image, 1, 0, p2);
  img_set_pixel(image, 0, 1, p3);
  img_set_pixel(image, 1, 1, p4);
  TEST_ASSERT_NOT_NULL(image);
  TEST_ASSERT_EQUAL(0, img_save_bmp("test_output.bmp", image));
  TEST_ASSERT_EQUAL(2, image->width);
  TEST_ASSERT_EQUAL(2, image->height);
  TEST_ASSERT_EQUAL(IMG_FMT_BGR24, image->format);
  TEST_ASSERT_NOT_NULL(image->data);
  TEST_ASSERT_EQUAL(6, image->stride); // 2 pixels * 3 bytes per pixel for BGR24

  img_t *loaded_image = img_load_bmp("test_output.bmp");
  TEST_ASSERT_NOT_NULL(loaded_image);
  TEST_ASSERT_EQUAL(2, loaded_image->width);
  TEST_ASSERT_EQUAL(2, loaded_image->height);
  TEST_ASSERT_EQUAL(IMG_FMT_BGR24, loaded_image->format);
  TEST_ASSERT_NOT_NULL(loaded_image->data);

  pixel_t out;
  img_get_pixel(loaded_image, 0, 0, &out);
  TEST_ASSERT_EQUAL_UINT8(255, out.r); // Red remains Red
  TEST_ASSERT_EQUAL_UINT8(0, out.g);   // Green remains Green
  TEST_ASSERT_EQUAL_UINT8(0, out.b);   // Blue remains Blue

  img_get_pixel(loaded_image, 1, 0, &out);
  TEST_ASSERT_EQUAL_UINT8(0, out.r);   // Red remains Red
  TEST_ASSERT_EQUAL_UINT8(255, out.g); // Green remains Green
  TEST_ASSERT_EQUAL_UINT8(0, out.b);   // Blue remains Blue

  img_get_pixel(loaded_image, 0, 1, &out);
  TEST_ASSERT_EQUAL_UINT8(0, out.r);   // Red remains Red
  TEST_ASSERT_EQUAL_UINT8(0, out.g);   // Green remains Green
  TEST_ASSERT_EQUAL_UINT8(255, out.b); // Blue remains Blue

  img_get_pixel(loaded_image, 1, 1, &out);
  TEST_ASSERT_EQUAL_UINT8(255, out.r); // Red remains Red
  TEST_ASSERT_EQUAL_UINT8(255, out.g); // Green remains Green
  TEST_ASSERT_EQUAL_UINT8(0, out.b);   // Blue remains Blue

  remove("test_output.bmp");
  img_destroy(image);
  img_destroy(loaded_image);
}

void test_img_save_bmp_gray_roundtrip() {
  img_t *image = img_create(2, 2, IMG_FMT_GRAY8);
  pixel_t p1 = {255, 255, 255, 255}; // White
  pixel_t p2 = {0, 0, 0, 255};       // Black
  pixel_t p3 = {128, 128, 128, 255}; // Gray
  pixel_t p4 = {64, 64, 64, 255};    // Dark Gray
  img_set_pixel(image, 0, 0, p1);
  img_set_pixel(image, 1, 0, p2);
  img_set_pixel(image, 0, 1, p3);
  img_set_pixel(image, 1, 1, p4);

  TEST_ASSERT_NOT_NULL(image);
  TEST_ASSERT_EQUAL(0, img_save_bmp("test_output_gray.bmp", image));

  img_t *loaded_image = img_load_bmp("test_output_gray.bmp");

  TEST_ASSERT_NOT_NULL(loaded_image);
  TEST_ASSERT_EQUAL(2, loaded_image->width);
  TEST_ASSERT_EQUAL(2, loaded_image->height);
  TEST_ASSERT_EQUAL(IMG_FMT_GRAY8, loaded_image->format);

  pixel_t out;

  img_get_pixel(loaded_image, 0, 0, &out);
  TEST_ASSERT_EQUAL_UINT8(255, out.r); // White remains White

  img_get_pixel(loaded_image, 1, 0, &out);
  TEST_ASSERT_EQUAL_UINT8(0, out.r); // Black remains Black

  img_get_pixel(loaded_image, 0, 1, &out);
  TEST_ASSERT_EQUAL_UINT8(128, out.r); // Gray remains Gray

  img_get_pixel(loaded_image, 1, 1, &out);
  TEST_ASSERT_EQUAL_UINT8(64, out.r); // Dark Gray remains Dark Gray

  remove("test_output_gray.bmp");
  img_destroy(image);
  img_destroy(loaded_image);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_img_save_bmp_null);
  RUN_TEST(test_img_save_bmp_bgr_roundtrip);
  RUN_TEST(test_img_save_bmp_gray_roundtrip);
  return UNITY_END();
}
