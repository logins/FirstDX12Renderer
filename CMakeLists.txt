cmake_minimum_required( VERSION 3.1 )

project(FirstDX12Renderer
	DESCRIPTION "Render simple things with DX12"
	LANGUAGES CXX
	)

if(NOT DEFINED MSVC)
	message(FATAL_ERROR "At the moment this program supports MSVC compilers only.")
endif()
	

set(DXC_BUILD_ARCH "x64")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/FindD3D12.cmake")

# Modules used by every part target
message("Found the following D3D12 Libraries: ${D3D12_LIB_NAMES}")


add_subdirectory(Part1)



