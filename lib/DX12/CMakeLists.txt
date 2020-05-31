cmake_minimum_required(VERSION 3.1)


set(DXC_BUILD_ARCH "x64")
include("${CMAKE_SOURCE_DIR}/cmake/FindD3D12.cmake")

# Modules used by every part target
message("Found the following D3D12 Libraries: ${D3D12_LIB_NAMES}")

# d3dx12 is just a header, so it becomes an Interface Library (it will not create build output)
add_library(d3dx12.h INTERFACE)
# We just include the directory of the header file as INTERFACE (so used only by the targets that will link this library)
target_include_directories(d3dx12.h 
	INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/D3DX12
	)