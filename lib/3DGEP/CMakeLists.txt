cmake_minimum_required(VERSION 2.8)

# The following will find every .h and .cpp file from Source folder
file(GLOB_RECURSE 3dgep_SRC
    #"Source/*.h"
    "Source/*.cpp",
    # TODO including platform specific code like D3D12 should be conditional when other Apis will be implemented 
)

add_library(3dgep ${3dgep_SRC})

list(APPEND 3DGEP_PUBLIC_INCLUDES
        ${CMAKE_CURRENT_LIST_DIR}/Source/Graphics/Public
        ${CMAKE_CURRENT_LIST_DIR}/Source/Public
)

# By setting PUBLIC_INCLUDE_DIRECTORIES property, external targets can retrieve the public include directories that we listed in 3DGEP_PUBLIC_INCLUDES
set_target_properties(3dgep PROPERTIES PUBLIC_INCLUDE_DIRECTORIES "${3DGEP_PUBLIC_INCLUDES}")

target_include_directories(3dgep #Note: Only the Public folder will be seen by the targets that will use this library!!
    PUBLIC
        ${3DGEP_PUBLIC_INCLUDES}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/Source
        # TODO including platform specific code like D3D12 should be conditional when other Apis will be implemented 
        ${CMAKE_CURRENT_LIST_DIR}/Source/Graphics/D3D12
)

target_link_libraries(3dgep d3d12.lib dxgi.lib d3dx12.h d3dcompiler.lib dxguid.lib eigen)