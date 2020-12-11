
add_executable(part4 Source/Part4.cpp "Source/Part4.h")

#Note: here we are linking 3dgep only, but more libraries will be implicitly linked with it, such as Eigen and d3d12.
target_link_libraries(part4 3dgep)

get_target_property(3DGEP_INTERFACE_INCLUDES 3dgep INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(EIGEN_INTERFACE_INCLUDES eigen INTERFACE_INCLUDE_DIRECTORIES)

target_include_directories( part3
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/Source
		3DGEP_INTERFACE_INCLUDES
		EIGEN_INTERFACE_INCLUDES
)

# Allowing reference of Part4 root path
target_compile_definitions(part4 
	PRIVATE 
		PART4_PROJ_ROOT_PATH=${CMAKE_CURRENT_SOURCE_DIR}
)
