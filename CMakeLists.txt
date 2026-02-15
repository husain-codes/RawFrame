cmake_minimum_required(VERSION 3.15)

project(RawFrame
    VERSION 0.1.0
    DESCRIPTION "Lightweight modular image processing library in pure C"
    LANGUAGES C
)

# Use C99
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Option to enable SDL display backend
#option(RAWFRAME_ENABLE_SDL "Enable SDL2 display backend" ON)

# Include public headers
include_directories(include)

# ===============================
# Core Library Sources
# ===============================

set(RAWFRAME_CORE
    src/core/img.c
    src/core/img_format.c
)

set(RAWFRAME_IO
    src/io/bmp_reader.c
    src/io/bmp_writer.c
)

set(RAWFRAME_PROCESSING
    src/processing/img_copy.c
    src/processing/img_grayscale.c
    src/processing/img_invert.c
    src/processing/img_blur.c
)

set(RAWFRAME_DISPLAY)

# ===============================
# SDL2 (Optional)
# ===============================

#if(RAWFRAME_ENABLE_SDL)
#    find_package(SDL2 REQUIRED)
#    list(APPEND RAWFRAME_DISPLAY
#        src/display/img_sdl_display.c
#    )
#endif()

# ===============================
# Create Library
# ===============================

add_library(rawframe STATIC
    ${RAWFRAME_CORE}
    ${RAWFRAME_IO}
    ${RAWFRAME_PROCESSING}
    ${RAWFRAME_DISPLAY}
)

target_include_directories(rawframe PUBLIC include)

if(RAWFRAME_ENABLE_SDL)
    target_link_libraries(rawframe PUBLIC SDL2::SDL2)
endif()

# ===============================
# Examples
# ===============================

#add_executable(example_load_display examples/example_load_display.c)
#target_link_libraries(example_load_display rawframe)

add_executable(image_copy examples/image_copy.c)
target_link_libraries(image_copy rawframe)

#add_executable(example_processing examples/example_processing.c)
#target_link_libraries(example_processing rawframe)
