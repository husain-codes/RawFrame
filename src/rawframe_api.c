#include "raw_frame.h"
#include <string.h>

#define RAWFRAME_MAKE_FOURCC(a, b, c, d)                                       \
    ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) |             \
     ((uint32_t)(d) << 24))
#define RAWFRAME_RAW10_PIXEL_FORMAT RAWFRAME_MAKE_FOURCC('p', 'R', 'A', 'A')

// Internal state hidden from C++
struct rawframe_ctx {
    v4l2_device_t *camera;
    isp_context_t *isp;
    int subdev_fd;

    uint16_t *isp_output_buffer;
    img_t *out_rgb;

    uint32_t cur_exp;
    uint32_t cur_again;
    uint32_t cur_dgain;
};

// Default initializers from your refactor
#define BAYER_10BIT_BLACK_LEVEL 64
#define INITIAL_EXPOSURE 400
#define INITIAL_ANALOG_GAIN 0
#define INITIAL_DIGITAL_GAIN 256

rawframe_ctx_t* rawframe_init(const char* video_node, const char* subdev_node, rawframe_config_t config) {
    if (!video_node || !subdev_node || config.width <= 0 || config.height <= 0) {
        return NULL;
    }

    rawframe_ctx_t *ctx = calloc(1, sizeof(rawframe_ctx_t));
    if (!ctx) return NULL;
    ctx->subdev_fd = -1;

    // 1. Open Hardware
    v4l2_config_t camera_config = {
        .device_path = video_node,
        .width = (uint32_t)config.width,
        .height = (uint32_t)config.height,
        .pixel_format = RAWFRAME_RAW10_PIXEL_FORMAT,
        .buffer_count = V4L2_REQ_BUFFER_COUNT
    };
    ctx->camera = v4l2_device_open(&camera_config);
    ctx->subdev_fd = open_v4l2_subdevice(subdev_node);
    if (!ctx->camera || ctx->subdev_fd < 0) {
        rawframe_cleanup(ctx);
        return NULL;
    }

    v4l2_capture_format_t capture_format;
    if (v4l2_device_get_format(ctx->camera, &capture_format) < 0) {
        rawframe_cleanup(ctx);
        return NULL;
    }

    // 2. Initialize ISP
    ctx->isp = isp_create();
    if (!ctx->isp) {
        rawframe_cleanup(ctx);
        return NULL;
    }
    isp_config_t isp_config = {
        .width = capture_format.width,
        .height = capture_format.height,
        .stride = capture_format.stride,
        .input_format = IMG_FMT_BAYER_10BIT,
        .output_format = IMG_FMT_RGB24,
        .black_level = BAYER_10BIT_BLACK_LEVEL,
        .enable_bls = true,
        .enable_awb = config.enable_awb,
        .enable_demosaic = true,
        .enable_gamma_correction = true,
        .enable_ccm = true,
        .enable_aec = config.enable_aec
    };

    if (isp_init(ctx->isp, &isp_config) < 0) {
        rawframe_cleanup(ctx);
        return NULL;
    }

    // 3. Allocate Buffers
    ctx->isp_output_buffer =
        malloc((size_t)capture_format.width * capture_format.height *
               sizeof(*ctx->isp_output_buffer));
    ctx->out_rgb =
        img_create(capture_format.width, capture_format.height, IMG_FMT_RGB24);
    if (!ctx->isp_output_buffer || !ctx->out_rgb) {
        rawframe_cleanup(ctx);
        return NULL;
    }

    // 4. Set Hardware Baselines
    ctx->cur_exp = INITIAL_EXPOSURE;
    ctx->cur_again = INITIAL_ANALOG_GAIN;
    ctx->cur_dgain = INITIAL_DIGITAL_GAIN;

    set_camera_control(ctx->subdev_fd, V4L2_CAMERA_CONTROL_EXPOSURE, ctx->cur_exp);
    set_camera_control(ctx->subdev_fd, V4L2_CAMERA_CONTROL_ANALOG_GAIN, ctx->cur_again);
    set_camera_control(ctx->subdev_fd, V4L2_CAMERA_CONTROL_DIGITAL_GAIN, ctx->cur_dgain);

    return ctx;
}

int rawframe_start(rawframe_ctx_t* ctx) {
    if (!ctx) return -1;

    if (v4l2_device_start(ctx->camera) < 0) return -1;
    if (isp_start(ctx->isp) < 0) return -1;

    return 0;
}

int rawframe_grab_rgb24(rawframe_ctx_t* ctx, uint8_t* out_rgb_buffer) {
    if (!ctx || !out_rgb_buffer) return -1;

    v4l2_frame_t frame = {0};

    // Dequeue frame from V4L2 (2 second timeout)
    if (v4l2_device_dqbuf(ctx->camera, &frame, 2) != 0) {
        return -1;
    }

    // Process through custom ISP
    if (isp_process_frame(ctx->isp, frame.data, ctx->isp_output_buffer, ctx->out_rgb, 0) < 0) {
        v4l2_device_qbuf(ctx->camera, frame.index);
        return -1;
    }

    // Dynamic Control Loops
    if (ctx->isp->config.enable_aec) {
        isp_pass_update_aec(ctx->subdev_fd, ctx->isp->current_luma,
                            &ctx->cur_exp, &ctx->cur_again, &ctx->cur_dgain);
    }

    // Copy the final rendered pixels out to the user's buffer (e.g., Qt)
    size_t rgb_size = ctx->out_rgb->stride[0] * ctx->out_rgb->height;
    memcpy(out_rgb_buffer, ctx->out_rgb->planes[0], rgb_size);

    // Requeue to hardware immediately
    v4l2_device_qbuf(ctx->camera, frame.index);

    return 0;
}

int rawframe_stop(rawframe_ctx_t* ctx) {
    if (!ctx) return -1;
    isp_stop(ctx->isp);
    // Note: v4l2_device_close internally stops the hardware stream
    return 0;
}

void rawframe_cleanup(rawframe_ctx_t* ctx) {
    if (!ctx) return;

    if (ctx->camera) v4l2_device_close(ctx->camera);
    if (ctx->subdev_fd >= 0) v4l2_subdevice_close(ctx->subdev_fd);
    if (ctx->isp) isp_destroy(ctx->isp);
    if (ctx->isp_output_buffer) free(ctx->isp_output_buffer);
    if (ctx->out_rgb) img_destroy(ctx->out_rgb);

    free(ctx);
}