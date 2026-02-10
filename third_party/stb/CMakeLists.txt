cmake_minimum_required(VERSION 3.10)
project(stb LANGUAGES C CXX)

# Set policy for target_sources with relative paths
if(POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
endif()

# Create an INTERFACE library for stb (header-only)
add_library(stb INTERFACE)
add_library(stb::stb ALIAS stb)

# Set the include directory
target_include_directories(stb INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Optionally, list all the header files for IDE support (using absolute paths)
set(STB_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_connected_components.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_c_lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_divide.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_ds.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_dxt.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_easy_font.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_herringbone_wang_tile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_hexwave.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_image.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_image_resize2.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_image_write.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_include.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_leakcheck.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_perlin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_rect_pack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_sprintf.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_textedit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_tilemap_editor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_truetype.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stb_voxel_render.h
)

# Add sources to the INTERFACE target for IDE visibility
target_sources(stb INTERFACE ${STB_HEADERS})

# Note: stb_vorbis.c is a special case - it's not header-only
# If you need to use stb_vorbis, you should compile it separately
