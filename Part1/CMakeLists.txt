
add_executable(part1 Source/Engine.cpp)

# Include directories for target part1.
# It will allow to include headers in C++ considering
# the following directories as root, e.g. #include <Helpers.h>
# since Helpers.h is inside Source folder.
target_include_directories( part1
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/Source
		${CMAKE_SOURCE_DIR}/lib/DX12/Include
)

target_link_libraries(part1 d3d12.lib dxgi.lib d3dx12.h)