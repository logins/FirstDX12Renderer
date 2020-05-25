cmake_minimum_required(VERSION 2.8)

set(DXC_BUILD_ARCH "x64")
include("${CMAKE_SOURCE_DIR}/cmake/FindD3D12.cmake")

# Modules used by every part target
message("Found the following D3D12 Libraries: ${D3D12_LIB_NAMES}")

add_subdirectory(3DGEP)