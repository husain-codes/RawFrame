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

---

## 💡 Requirements

To build **RawFrame** you'll need the following tools installed on your system:

- **CMake** (version 3.10 or newer)
- A C compiler (e.g. `gcc`, `clang`, or MSVC on Windows)
- Standard build tools (`make` on Unix-like systems, or the appropriate generator for CMake)

### Installing CMake

On Ubuntu/Debian:

```sh
sudo apt update
sudo apt install cmake
```

On Fedora/CentOS/RHEL:

```sh
sudo dnf install cmake    # or yum on older releases
```

On Windows, download the installer from the [CMake website](https://cmake.org/download/) or use `choco install cmake` if you have Chocolatey.

On macOS, use `brew install cmake` with Homebrew.

Ensure `cmake --version` prints a valid version before continuing.

---

## 🛠️ Building the Library

The project uses a conventional out-of-source build using CMake. From the repository root run:

```sh
mkdir -p build && cd build
cmake ..              # configure the project; you may specify a different generator
cmake --build .       # builds the default target (usually 'all')
```

If you are on Unix-like systems you can also use `make` after configuration:

```sh
make                  # compile all targets
```

To build the example programs or run tests, use:

```sh
cmake --build . --target example_copy
cmake --build . --target image_copy
cmake --build . --target tests
```

> Binaries and sample BMP files will appear in the `build/` directory once the targets complete.
