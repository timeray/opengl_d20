cmake_minimum_required(VERSION 3.15)

# Set project name
project(OpenGL_D20 VERSION 1.0)

add_executable(d20 d20.c "src/icosahedron.c" "src/animation.c" "src/scene.c" "src/text.c" "src/shader.c")

# Copying required data
add_custom_command(
	TARGET d20 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders
	COMMENT "Copying shader files" VERBATIM
)
add_custom_command(
	TARGET d20 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/textures ${CMAKE_CURRENT_BINARY_DIR}/textures
	COMMENT "Copying texture files" VERBATIM
)
add_custom_command(
	TARGET d20 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/fonts ${CMAKE_CURRENT_BINARY_DIR}/fonts
	COMMENT "Copying fonts files" VERBATIM
)


add_library(d20_compiler_flags INTERFACE)
target_compile_features(d20_compiler_flags INTERFACE c_std_11)

target_include_directories(d20 PUBLIC ${CMAKE_SOURCE_DIR}/include)

include(FindOpenGL)

find_package(glfw3 3.3 REQUIRED)

target_link_libraries(d20 PUBLIC d20_compiler_flags OpenGL::GL glfw cglm_headers glad freetype)
add_subdirectory(glad EXCLUDE_FROM_ALL)
add_subdirectory(cglm EXCLUDE_FROM_ALL)
add_subdirectory(freetype EXCLUDE_FROM_ALL)
