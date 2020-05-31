cmake_minimum_required(VERSION 2.8)

# The following will find every .h and .cpp file from Source folder
file(GLOB 3dgep_SRC
    #"Source/*.h"
    "Source/*.cpp"
)

add_library(3dgep ${3dgep_SRC})

target_include_directories(3dgep #Note: Only the Public folder will be seen by the targets that will use this library!!
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/Source/Public
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/Source
)

target_link_libraries(3dgep d3d12.lib dxgi.lib d3dx12.h)