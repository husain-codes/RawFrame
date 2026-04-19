#include <io/img_io.h>
#include <unity.h>

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void test_img_create_nv12_success() {
  uint32_t width = 4;
  uint32_t height = 4;
  img_t *img = img_create(width, height, IMG_FMT_NV12);
  TEST_ASSERT_NOT_NULL(img);
  TEST_ASSERT_EQUAL(width, img->width);
  TEST_ASSERT_EQUAL(height, img->height);
  TEST_ASSERT_EQUAL(IMG_FMT_NV12, img->format);
  TEST_ASSERT_EQUAL(2, img->num_planes);
  TEST_ASSERT_NOT_NULL(img->planes[0]);
  TEST_ASSERT_NOT_NULL(img->planes[1]);
  TEST_ASSERT_NULL(img->planes[2]); // NV12 should have only 2 planes
  TEST_ASSERT_EQUAL(width, img->stride[0]);
  TEST_ASSERT_EQUAL(width, img->stride[1]);
  img_destroy(img);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_img_create_nv12_success);
  return UNITY_END();
}