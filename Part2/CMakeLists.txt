
add_executable(part2 Source/Part2.cpp)

#Note: here we are linking 3dgep only, but more libraries will be implicitly linked with it, such as Eigen and d3d12.
target_link_libraries(part2 3dgep) 

# Include directories for target part2.
# It will allow to include headers in C++ considering
# the following directories as root, e.g. #include <Helpers.h>
# since Helpers.h is inside Source folder.

get_target_property(3DGEP_INTERFACE_INCLUDES 3dgep INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(EIGEN_INTERFACE_INCLUDES eigen INTERFACE_INCLUDE_DIRECTORIES)

target_include_directories( part2
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/Source
		3DGEP_INTERFACE_INCLUDES
		EIGEN_INTERFACE_INCLUDES
)


# Allowing reference of Part2 root path
target_compile_definitions(part2 PRIVATE PART2_PROJ_ROOT_PATH=${CMAKE_CURRENT_SOURCE_DIR})