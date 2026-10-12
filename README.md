# RawFrame

![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)
![CI](https://github.com/husain-codes/RawFrame/actions/workflows/cmake-single-platform.yml/badge.svg)

RawFrame is a full-stack, modular embedded Linux camera pipeline built specifically for the **Raspberry Pi 5**. 

It bridges the gap between low-level kernel space and high-level application rendering by providing a complete ecosystem: **Custom V4L2 Kernel Driver ➔ V4L2 Hardware Capture ➔ Custom C ISP Engine ➔ C++/Qt/OpenGL Application Layer**, all within a single monolithic repository.
---

## 🎯 The Purpose

RawFrame has evolved into a complete learning ecosystem for embedded vision and camera architectures. It is designed for developers who want to understand exactly how raw photons hit a sensor (like the IMX219), how a custom Linux kernel driver registers the hardware, how the data travels through the V4L2 subsystem, gets mathematically processed (Custom ISP), and finally renders on a UI (Qt/OpenGL)—all without relying on opaque black-box vendor blobs.

---

## 🚀 Key Features
- **Custom V4L2 Kernel Driver:** Writing a custom Linux kernel module from scratch to interface with the camera sensor and the MIPI CSI-2 receiver.
- **Direct V4L2 Integration:** Direct user-space communication with `/dev/videoX` to allocate kernel buffers, map memory, and capture raw MIPI CSI-2 frames.
- **Custom Image Signal Processor (ISP):**
  - 10-bit packed MIPI (`pRAA`) to 16-bit unpacking.
  - Black Level Subtraction (BLS) to fix sensor floor noise.
  - Auto White Balance (AWB) using the Grey World assumption with 16-bit precision.
  - High-quality Bayer Demosaicing (RGGB to RGB24).
- **Zero-Copy Architecture:** Wraps V4L2 kernel memory directly into the `img_t` abstraction to avoid expensive memory copies before ISP processing.
- **C++/Qt Application Layer:** A front-end orchestrator for real-time OpenGL rendering of the processed ISP frames.
- **Legacy Format Support:** Retains memory-safe buffer management and conversions for planar YUV, NV12, and standard RGB/BGR formats.

---
## 📸 Supported Camera Formats

| Format | Description |
|---|---|
| `pRAA` (10-bit MIPI) | V4L2 packed raw Bayer format (4 pixels in 5 bytes). |
| `BAYER_16BIT` | Unpacked 16-bit intermediate representation for high-precision math. |
| `BAYER_RGGB` | 8-bit demosaic-ready raw grid. |
| `RGB24` / `BGR24` | Packed 24-bit output formats for Qt/OpenGL rendering. |
| `YUV420P` / `NV12` | Planar formats for video encoding. |
| `PPM` / `BMP` | Native file I/O for debugging and saving frames. |

---

## 🧱 Pipeline Architecture

RawFrame follows a strict separation of concerns across four distinct layers:

### 1. Kernel Layer (C)
- Custom V4L2 subdevice driver development to initialize the sensor, configure MIPI lanes, and expose controls to user-space.

### 2. Hardware Capture Layer (C)
- `v4l2_capture.c`: Handles `VIDIOC_REQBUFS`, `VIDIOC_QBUF`, and `VIDIOC_DQBUF`. Auto-discovers the correct video node for the active sensor and manages the streaming state machine.

### 3. Middleware / ISP Layer (C)
- `isp_pipeline.c`: The mathematical heart of the camera. Takes raw packed bytes, unpacks them to a 16-bit `img_t` buffer, applies BLS and AWB, scales to 8-bit, and demosaics into a final RGB image.

### 4. Application Layer (C++ / Qt)
- The orchestrator. It holds the `rawframe_handle_t`, calls the frame grabber, and pushes the final RGB frames to an OpenGL texture for zero-latency display.

---

## ✅ Supported Operations

- `img_load_bmp()` / `img_save_bmp()`
- `img_create()` / `img_destroy()`
- `img_toggle_rgb_bgr()`
- `img_convert_rgb_to_gray()`
- `img_convert_rgb_to_yuv444()` / `img_convert_yuv444_to_rgb()`
- `img_convert_rgb_to_nv12()` / `img_convert_nv12_to_rgb()`
- `img_convert_rgb_to_yuv420p()` / `img_convert_yuv420p_to_rgb()`
- `img_copy()` / `img_invert()` / `img_blur()`

---

## 💻 Example: The Capture Loop

```c
#include <io/v4l2_capture.h>
#include <img/isp_pipeline.h>

int main() {
    // 1. Initialize Hardware
    v4l2_config_t hw_config = { .width = 1640, .height = 1232, .pixel_format = V4L2_PIX_FMT_SRGGB10P };
    v4l2_device_t *camera = v4l2_device_open(&hw_config);
    v4l2_device_start(camera);

    // 2. Initialize ISP
    isp_context_t *isp = isp_create();
    isp_init(isp, &isp_config);
    isp_start(isp);

    // 3. The Real-Time Loop
    v4l2_frame_t frame = {0};
    if (v4l2_device_dqbuf(camera, &frame, 2) == 0) {

        // Unpack, BLS, AWB, and Demosaic directly from kernel memory
        isp_process_frame(isp, frame.data, output_rgb_buffer, frame_id);

        // Return memory to kernel
        v4l2_device_qbuf(camera, frame.index);
    }

    // 4. Teardown
    isp_destroy(isp);
    v4l2_device_close(camera);
    return 0;
}
```c

---

## 🧱 Architecture

RawFrame follows a layered structure with a strong separation of concerns:

### Core Layer
- `img_t` definition with `width`, `height`, `format`, `stride[]`, and `planes[]`
- Memory allocation and destruction for packed and planar images
- Metadata validation and format helpers

### IO Layer
- BMP image reader and writer
- File-based image load/save support for validation and examples

### Processing Layer
- Pixel-format conversion routines
- Buffer-copy and image transform helpers
- Per-format image operations

### Test Layer
- Unity framework integration in `tests/`
- Conversion and regression tests for RGB, YUV, NV12, and YUV420P
- CI-friendly test targets via CMake

---

## 💡 Requirements

To build RawFrame you'll need:

- CMake 3.10 or newer
- A C compiler (`gcc`, `clang`, or MSVC)
- Standard build tools (`make`, `ninja`, or CMake generator-specific tools)

---

## 🛠️ Building

From the repository root:

```sh
mkdir -p build && cd build
cmake ..
cmake --build .
```

To build examples and tests:

```sh
cmake --build . --target example_convert
cmake --build . --target tests
```

To run the Unity tests after building:

```sh
ctest --test-dir .
```

---

## 📦 License

MIT License. See `LICENSE`.
