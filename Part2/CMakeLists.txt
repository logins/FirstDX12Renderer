
add_executable(part2 Source/Part2.cpp)

# Include directories for target part1.
# It will allow to include headers in C++ considering
# the following directories as root, e.g. #include <Helpers.h>
# since Helpers.h is inside Source folder.
target_include_directories( part2
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/Source
		${CMAKE_SOURCE_DIR}/lib/DX12/Include
		${CMAKE_SOURCE_DIR}/lib/3DGEP/Source
)

target_link_libraries(part2 d3d12.lib dxgi.lib 3dgep)