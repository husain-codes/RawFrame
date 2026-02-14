# RawFrame

RawFrame is a lightweight, modular image processing library written in pure C.

It provides a clean abstraction over raw pixel buffers and is designed to help developers understand low-level image representation, memory layout, and basic image processing concepts.

The project follows a layered architecture separating core image management, file I/O, processing algorithms, and display backends.

---

## 🚀 Features

- Image object abstraction (`img_t`)
- Controlled memory lifecycle (create / destroy)
- Support for BMP image input and output
- Basic image operations (copy, invert, grayscale)
- Modular and extensible design

---

## 🧱 Architecture

RawFrame follows a layered structure:

### Core Layer
- Image structure definition
- Memory allocation and destruction
- Metadata management

### IO Layer
- BMP reader
- BMP writer

### Processing Layer
- Image copy
- Grayscale conversion
- Inversion
- Blur (planned)

### Display Layer
- SDL2-based rendering backend

The core layer is independent and does not depend on external libraries.

---

## Image Representation

An image in RawFrame consists of:

Metadata + Continuous Pixel Buffer

Each image object stores:

- Width
- Height
- Pixel format
- Stride
- Pointer to pixel data

Pixel data is stored in row-major order.
