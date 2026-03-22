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

void test_img_convert_rgb_to_gray_null() {
    TEST_ASSERT_EQUAL(-1, img_convert_rgb_to_gray(NULL));
}

void test_img_convert_rgb_to_gray_wrong_format() {
    img_t *img = img_create(10, 10, IMG_FMT_BGR24);
    TEST_ASSERT_NOT_NULL(img);
    TEST_ASSERT_EQUAL(-1, img_convert_rgb_to_gray(img));
    img_destroy(img);
}

void test_img_convert_rgb_to_gray_success() {
    img_t *img = img_create(2, 2, IMG_FMT_RGB24);
    TEST_ASSERT_NOT_NULL(img);

    // Set known pixel values
    pixel_t p1 = {255, 0, 0, 255}; // Red
    pixel_t p2 = {0, 0, 0, 255}; // Black
    pixel_t p3 = {255, 255, 255, 255}; // White
    pixel_t p4 = {255, 255, 0, 255}; // Yellow

    img_set_pixel(img, 0, 0, p1);
    img_set_pixel(img, 1, 0, p2);
    img_set_pixel(img, 0, 1, p3);
    img_set_pixel(img, 1, 1, p4);

    // Convert RGB to Grayscale
    TEST_ASSERT_EQUAL(0, img_convert_rgb_to_gray(img));
    TEST_ASSERT_EQUAL(IMG_FMT_GRAY8, img->format); // Format should now be Grayscale
    TEST_ASSERT_EQUAL(img->width, img->stride); // Stride should now equal width for grayscale

    // Verify the pixel values are correctly converted to grayscale
    pixel_t out;
    img_get_pixel(img, 0, 0, &out);
    TEST_ASSERT_EQUAL_UINT8(76, out.r); // Grayscale value for Red
    TEST_ASSERT_EQUAL_UINT8(76, out.g); // Grayscale value for Red
    TEST_ASSERT_EQUAL_UINT8(76, out.b); // Grayscale value for Red

    img_get_pixel(img, 1, 0, &out);
    TEST_ASSERT_EQUAL_UINT8(0, out.r); // Grayscale value for Black
    TEST_ASSERT_EQUAL_UINT8(0, out.g); // Grayscale value for Black
    TEST_ASSERT_EQUAL_UINT8(0, out.b); // Grayscale value for Black

    img_get_pixel(img, 0, 1, &out);
    TEST_ASSERT_EQUAL_UINT8(255, out.r); // Grayscale value for White
    TEST_ASSERT_EQUAL_UINT8(255, out.g); // Grayscale value for White
    TEST_ASSERT_EQUAL_UINT8(255, out.b); // Grayscale value for White

    img_get_pixel(img, 1, 1, &out);
    TEST_ASSERT_EQUAL_UINT8(225, out.r); // Grayscale value for Yellow
    TEST_ASSERT_EQUAL_UINT8(225, out.g); // Grayscale value for Yellow
    TEST_ASSERT_EQUAL_UINT8(225, out.b); // Grayscale value for Yellow
    img_destroy(img);
}

void test_img_convert_rgb_to_yuv444_null() {
    TEST_ASSERT_EQUAL(-1, img_convert_rgb_to_yuv444(NULL));
}

void test_img_convert_rgb_to_yuv444_wrong_format() {
    img_t *img = img_create(10, 10, IMG_FMT_BGR24);
    TEST_ASSERT_NOT_NULL(img);
    TEST_ASSERT_EQUAL(-1, img_convert_rgb_to_yuv444(img));
    img_destroy(img);
}

void test_img_convert_yuv444_to_rgb_null() {
    TEST_ASSERT_EQUAL(-1, img_convert_yuv444_to_rgb(NULL));
}

void test_img_convert_yuv444_to_rgb_wrong_format() {
    img_t *img = img_create(10, 10, IMG_FMT_BGR24);
    TEST_ASSERT_NOT_NULL(img);
    TEST_ASSERT_EQUAL(-1, img_convert_yuv444_to_rgb(img));
    img_destroy(img);
}

void test_img_convert_rgb_to_yuv444_round_trip_success() {
    img_t *img = img_create(2, 2, IMG_FMT_RGB24);
    TEST_ASSERT_NOT_NULL(img);

    // Set known pixel values
    pixel_t p[4] = {{255, 0, 0, 255},
                    {0, 255, 0, 255},
                    {0, 0, 255, 255},
                    {255, 255, 0, 255}}; // Red

    img_set_pixel(img, 0, 0, p[0]);
    img_set_pixel(img, 1, 0, p[1]);
    img_set_pixel(img, 0, 1, p[2]);
    img_set_pixel(img, 1, 1, p[3]);

    // Convert RGB to YUV444
    TEST_ASSERT_EQUAL(0, img_convert_rgb_to_yuv444(img));
    TEST_ASSERT_EQUAL(IMG_FMT_YUV444, img->format); // Format should now be YUV444

    // Verify the pixel values are correctly converted to YUV444
    uint8_t *pixel;
    uint8_t Y, U, V;
    int m=0;
    for(int y=0; y<2; y++){
        for(int x=0; x<2; x++){
            pixel = img->data + y * img->stride + x * 3; // Access raw data for YUV values
            Y = (uint8_t)(0.299 * p[m].r + 0.587 * p[m].g + 0.114 * p[m].b);
            U = (uint8_t)(-0.169 * p[m].r - 0.331 * p[m].g + 0.5 * p[m].b +
                            128);
            V = (uint8_t)(0.5 * p[m].r - 0.419 * p[m].g - 0.081 * p[m].b + 128);
            TEST_ASSERT_INT_WITHIN(2, pixel[0], Y);   // Y value for Red
            TEST_ASSERT_INT_WITHIN(2, pixel[1], U);   // U value for Red
            TEST_ASSERT_INT_WITHIN(2, pixel[2], V); // V value for Red
            m++;
        }
    }

    // Verify round trip conversion back to RGB
    TEST_ASSERT_EQUAL(0, img_convert_yuv444_to_rgb(img));
    TEST_ASSERT_EQUAL(IMG_FMT_RGB24, img->format); // Format should now be RGB
    pixel_t out;
    img_get_pixel(img, 0, 0, &out);
    TEST_ASSERT_INT_WITHIN(2,255, out.r); // Red remains Red
    TEST_ASSERT_INT_WITHIN(2,0, out.g);   // Green remains Green
    TEST_ASSERT_INT_WITHIN(2,0, out.b);   // Blue remains Blue

    img_get_pixel(img, 1, 0, &out);
    TEST_ASSERT_INT_WITHIN(2,0, out.r);   // Red remains Red
    TEST_ASSERT_INT_WITHIN(2,255, out.g); // Green remains Green
    TEST_ASSERT_INT_WITHIN(2,0, out.b);   // Blue remains Blue

    img_get_pixel(img, 0, 1, &out);
    TEST_ASSERT_INT_WITHIN(2,0, out.r);   // Red remains Red
    TEST_ASSERT_INT_WITHIN(2,0, out.g);   // Green remains Green
    TEST_ASSERT_INT_WITHIN(2,255, out.b); // Blue remains Blue

    img_get_pixel(img, 1, 1, &out);
    TEST_ASSERT_INT_WITHIN(2,255, out.r); // Red remains Red
    TEST_ASSERT_INT_WITHIN(2,255, out.g); // Green remains Green
    TEST_ASSERT_INT_WITHIN(2,0, out.b);   // Blue remains Blue

    img_destroy(img);
}


int main(){
    UNITY_BEGIN();
    RUN_TEST(test_img_toggle_rgb_bgr_null);
    RUN_TEST(test_img_toggle_rgb_bgr_wrong_format);
    RUN_TEST(test_img_toggle_rgb_bgr_success);

    RUN_TEST(test_img_convert_rgb_to_gray_null);
    RUN_TEST(test_img_convert_rgb_to_gray_wrong_format);
    RUN_TEST(test_img_convert_rgb_to_gray_success);

    RUN_TEST(test_img_convert_rgb_to_yuv444_null);
    RUN_TEST(test_img_convert_rgb_to_yuv444_wrong_format);
    RUN_TEST(test_img_convert_yuv444_to_rgb_null);
    RUN_TEST(test_img_convert_yuv444_to_rgb_wrong_format);

    RUN_TEST(test_img_convert_rgb_to_yuv444_round_trip_success);
    return UNITY_END();
}