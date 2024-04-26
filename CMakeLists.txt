cmake_minimum_required(VERSION 3.15)

# Set project name
project(OpenGL_D20 VERSION 1.0)

add_executable(d20 d20.c)
add_custom_command(
	TARGET d20 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:d20>/shaders
	COMMENT "Copying shader files" VERBATIM
	)
# add_dependencies(d20 copy_shader_files)

add_library(d20_compiler_flags INTERFACE)
target_compile_features(d20_compiler_flags INTERFACE c_std_11)

include(FindOpenGL)

find_package(glfw3 3.3 REQUIRED)

target_link_libraries(d20 PUBLIC d20_compiler_flags OpenGL::GL glfw cglm_headers glad)
add_subdirectory(glad EXCLUDE_FROM_ALL)
add_subdirectory(cglm EXCLUDE_FROM_ALL)

# file(COPY ${PROJECT_SOURCE_DIR}/vertex_shader.glsl DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
