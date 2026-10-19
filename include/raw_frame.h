#pragma once
#ifndef RAWFRAME_H
#define RAWFRAME_H

#include <img/img.h>
#include <io/img_io.h>
#include <img/img_convert.h>
#include <img/img_format.h>
#include <img/img_processing.h>
#include <img/img_types.h>
#include <img/img_convert.h>
#include <img/img_processing.h>
#include <img/isp_pipeline.h>
#include <io/v4l2_capture.h>
#include <img/isp_pipeline.h>
#include <io/img_io.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to hide internal ISP/V4L2 state from C++
typedef struct rawframe_ctx rawframe_ctx_t;

// Simple configuration passed from the Qt App
typedef struct {
    int width;
    int height;
    bool enable_aec;
    bool enable_awb;
} rawframe_config_t;

// The 5 Core API Functions
rawframe_ctx_t* rawframe_init(const char* video_node, const char* subdev_node, rawframe_config_t config);
int rawframe_start(rawframe_ctx_t* ctx);

// Qt calls this in a loop to get a fully processed RGB buffer
int rawframe_grab_rgb24(rawframe_ctx_t* ctx, uint8_t* out_rgb_buffer);

int rawframe_stop(rawframe_ctx_t* ctx);
void rawframe_cleanup(rawframe_ctx_t* ctx);
#ifdef __cplusplus
}
#endif

#endif // RAWFRAME_H