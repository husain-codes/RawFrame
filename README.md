# RawFrame

![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)
![CI](https://github.com/husain-codes/RawFrame/actions/workflows/cmake-single-platform.yml/badge.svg)

RawFrame is a lightweight, modular image processing library written in pure C.

It provides a clean abstraction over raw pixel buffers and multi-plane image layouts used by camera pipelines and embedded vision systems.

This library is built for low-level image format conversion, memory-safe buffer management, and simple testing in automotive, Android HAL, and ISP learning projects.

---

## Why RawFrame

RawFrame is designed as a camera pipeline learning library for developers who want to understand raw image layouts, planar YUV, and format conversion used in Android camera HAL and automotive ISP work. It keeps the core clean, uses explicit `img_t` metadata, and exposes both packed and multi-plane image surfaces.

---

## 🚀 Features

- `img_t` image object abstraction with metadata, format, and per-plane pointers
- Multi-plane support for YUV formats, including `planes[3]` and per-plane stride control
- BMP file I/O for image load/save
- RGB/BGR byte-order toggle and packed pixel format manipulation
- Color conversion: RGB ↔ BGR, RGB ↔ GRAY, RGB ↔ YUV444, RGB ↔ NV12, RGB ↔ YUV420P
- Image processing: copy, invert, grayscale, blur
- Unity-based tests covering conversion and buffer layout behavior
- Modular architecture for core, IO, processing, and test layers

---

## Supported Pixel Formats

| Format | Description |
|---|---|
| `RGB24` | Packed 24-bit RGB (R,G,B) |
| `BGR24` | Packed 24-bit BGR (B,G,R) |
| `GRAY8` | 8-bit grayscale |
| `PPM` | Portable Pixmap format loader/saver |
| `YUV444` | Full-resolution planar YUV |
| `NV12` | 4:2:0 semiplanar YUV with interleaved UV plane |
| `YUV420P` | 4:2:0 planar YUV with separate Y, U, V planes |

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

## Example

```c
#include <stdio.h>
#include <img/img.h>
#include <img/img_convert.h>

int main(void) {
    img_t *img = img_load_bmp("frame.bmp");
    if (!img) {
        fprintf(stderr, "Failed to load frame.bmp\n");
        return 1;
    }

    if (img_toggle_rgb_bgr(img) != 0) {
        fprintf(stderr, "Failed to toggle RGB/BGR\n");
        img_destroy(img);
        return 1;
    }

    img_t *nv12 = img_convert_rgb_to_nv12(img);
    if (!nv12) {
        fprintf(stderr, "RGB to NV12 conversion failed\n");
        img_destroy(img);
        return 1;
    }

    printf("Converted image to NV12: %dx%d\n", nv12->width, nv12->height);
    img_destroy(nv12);
    img_destroy(img);
    return 0;
}
```

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
