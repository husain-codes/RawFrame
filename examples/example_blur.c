#include <raw_frame.h>
#include <stdio.h>
#define KSIZE 31

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.bmp>\n", argv[0]);
        return -1;
    }
    img_t *src = img_load_bmp(argv[1]);
    img_t *dst = img_create(src->width, src->height, src->format);
    int status = img_box_blur(src, dst, KSIZE);
    img_save_bmp("output_blur.bmp", dst);

    // Convert BGR to NV12 and do blurring and convert back to BGR and save
    if(img_toggle_rgb_bgr(src) != 0) {
        fprintf(stderr, "Failed to toggle BGR to RGB\n");
        return -1;
    }
    
    img_t *nv12_img = img_convert_rgb_to_nv12(src);
    img_t *blurred_nv12 = img_create(nv12_img->width, nv12_img->height, nv12_img->format);
    if (img_box_blur(nv12_img, blurred_nv12, KSIZE) != 0) {
        fprintf(stderr, "Failed to blur NV12 image\n");
        return -1;
    }
    img_t *blurred_rgb = img_convert_nv12_to_rgb(blurred_nv12);
    if(img_toggle_rgb_bgr(blurred_rgb) != 0) {
        fprintf(stderr, "Failed to toggle RGB to BGR\n");
        return -1;
    }
    if(img_save_bmp("output_blur_nv12.bmp", blurred_rgb) != 0) {
        fprintf(stderr, "Failed to save blurred RGB image\n");
        return -1;
    }
    // for yuv420p
    img_t *yuv420p_img = img_convert_rgb_to_yuv420p(src);
    img_t *blurred_yuv420p = img_create(yuv420p_img->width, yuv420p_img->height, yuv420p_img->format);
    if (img_box_blur(yuv420p_img, blurred_yuv420p, KSIZE) != 0) {
        fprintf(stderr, "Failed to blur YUV420P image\n");
        return -1;
    }
    img_t *blurred_rgb_from_yuv420p = img_convert_yuv420p_to_rgb(blurred_yuv420p);
    if(img_toggle_rgb_bgr(blurred_rgb_from_yuv420p) != 0) {
        fprintf(stderr, "Failed to toggle RGB to BGR\n");
        return -1;
    }
    if(img_save_bmp("output_blur_yuv420p.bmp", blurred_rgb_from_yuv420p) != 0) {
        fprintf(stderr, "Failed to save blurred RGB image from YUV420P\n");
        return -1;
    }
    // Cleanup
    img_destroy(src);
    img_destroy(dst);
    img_destroy(nv12_img);
    img_destroy(blurred_nv12);
    img_destroy(blurred_rgb);
    img_destroy(yuv420p_img);
    img_destroy(blurred_yuv420p);
    img_destroy(blurred_rgb_from_yuv420p);
    return 0;
}