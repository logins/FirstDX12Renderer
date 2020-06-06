
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
		${CMAKE_SOURCE_DIR}/lib/eigen/Eigen/src
)

target_link_libraries(part2 3dgep eigen)

# Allowing reference of Part2 root path
target_compile_definitions(part2 PRIVATE PART2_PROJ_ROOT_PATH=${CMAKE_CURRENT_SOURCE_DIR})