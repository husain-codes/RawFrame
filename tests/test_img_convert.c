#include <unity.h>
#include <img/img_convert.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_img_toggle_rgb_bgr_null() {
    TEST_ASSERT_EQUAL(-1, img_toggle_rgb_bgr(NULL));
}

void test_img_toggle_rgb_bgr_wrong_format() {
    img_t *img = img_create(10, 10, IMG_FMT_GRAY8);
    TEST_ASSERT_NOT_NULL(img);
    TEST_ASSERT_EQUAL(-1, img_toggle_rgb_bgr(img));
    img_destroy(img);
}
void test_img_toggle_rgb_bgr_success() {
    img_t *img = img_create(2, 2, IMG_FMT_RGB24);
    TEST_ASSERT_NOT_NULL(img);

    // Set known pixel values
    pixel_t p1 = {255, 0, 0, 255}; // Red
    pixel_t p2 = {0, 255, 0, 255}; // Green
    pixel_t p3 = {0, 0, 255, 255}; // Blue
    pixel_t p4 = {255, 255, 0, 255}; // Yellow

    img_set_pixel(img, 0, 0, p1);
    img_set_pixel(img, 1, 0, p2);
    img_set_pixel(img, 0, 1, p3);
    img_set_pixel(img, 1, 1, p4);

    // Toggle RGB to BGR
    TEST_ASSERT_EQUAL(0, img_toggle_rgb_bgr(img));
    TEST_ASSERT_EQUAL(IMG_FMT_BGR24, img->format); // Format should now be BGR

    // Verify the pixel values remain unchanged (img_get_pixel returns RGB regardless of format)
    pixel_t out;
    img_get_pixel(img, 0, 0, &out);
    TEST_ASSERT_EQUAL_UINT8(255, out.r); // Red remains Red
    TEST_ASSERT_EQUAL_UINT8(0, out.g);   // Green remains Green
    TEST_ASSERT_EQUAL_UINT8(0, out.b);   // Blue remains Blue

    img_get_pixel(img, 1, 0, &out);
    TEST_ASSERT_EQUAL_UINT8(0, out.r);   // Red remains Red
    TEST_ASSERT_EQUAL_UINT8(255, out.g); // Green remains Green
    TEST_ASSERT_EQUAL_UINT8(0, out.b);   // Blue remains Blue

    img_get_pixel(img, 0, 1, &out);
    TEST_ASSERT_EQUAL_UINT8(0, out.r);   // Red remains Red
    TEST_ASSERT_EQUAL_UINT8(0, out.g);   // Green remains Green
    TEST_ASSERT_EQUAL_UINT8(255, out.b); // Blue remains Blue

    img_get_pixel(img, 1, 1, &out);
    TEST_ASSERT_EQUAL_UINT8(255, out.r); // Red remains Red
    TEST_ASSERT_EQUAL_UINT8(255, out.g); // Green remains Green
    TEST_ASSERT_EQUAL_UINT8(0, out.b);   // Blue remains Blue

    // Rount trip toggle back to RGB
    TEST_ASSERT_EQUAL(0, img_toggle_rgb_bgr(img));
    TEST_ASSERT_EQUAL(IMG_FMT_RGB24, img->format); // Format should now be RGB
    img_destroy(img);
}

int main(){
    UNITY_BEGIN();
    RUN_TEST(test_img_toggle_rgb_bgr_null);
    RUN_TEST(test_img_toggle_rgb_bgr_wrong_format);
    RUN_TEST(test_img_toggle_rgb_bgr_success);
    return UNITY_END();
}