cmake_minimum_required(VERSION 3.16)

# The following will find every .h and .cpp file from Source folder
file(GLOB_RECURSE 3dgep_SRC
    #"Source/*.h"
    "Source/*.cpp",
    # TODO including platform specific code like D3D12 should be conditional when other Apis will be implemented 
)

add_library(3dgep ${3dgep_SRC})

target_include_directories(3dgep #Note: Only the Public folder will be seen by the targets that will use this library!!
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/Source/Graphics/Public
        ${CMAKE_CURRENT_LIST_DIR}/Source/Public
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/Source
        ${CMAKE_CURRENT_LIST_DIR}/Source/Graphics
        # TODO including platform specific code like D3D12 should be conditional when other Apis will be implemented 
        ${CMAKE_CURRENT_LIST_DIR}/Source/Graphics/D3D12
)

# Note: All the following libraries are implicitly linked PUBLIC, meaning that they will be part of the linking interface.
# That means that all the projects that are linking 3dgep will also be able to use these libraries.
# For a more proper engine application, we want parts like d3d12 libraries to be PRIVATE and used inside the graphics module only.
target_link_libraries(3dgep
    PUBLIC
        d3d12.lib dxgi.lib d3dx12.h d3dcompiler.lib dxguid.lib eigen
    PRIVATE
        DirectXTex
)


target_compile_definitions(3dgep 
	PRIVATE
		GRAPHICS_SDK_D3D12=1 # This define simulates a switch between possible graphics APIs. When defined, code will assume we chose D3D12
)


target_precompile_headers(3dgep
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/Source/GepPCH.h
)
