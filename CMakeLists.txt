cmake_minimum_required( VERSION 3.1 )

project(FirstDX12Renderer
	DESCRIPTION "Render simple things with DX12"
	LANGUAGES CXX
	)

if(NOT DEFINED MSVC)
	message(FATAL_ERROR "At the moment this program supports MSVC compilers only.")
endif()
	

add_subdirectory(lib)

add_subdirectory(Part1)

add_subdirectory(Part2)

add_subdirectory(Part3)


