cmake_minimum_required(VERSION 3.15)

# Set project name
project(OpenGL_D20 VERSION 1.0)

add_executable(d20 d20.c "src/icosahedron.c" "src/animation.c" "src/scene.c" "src/text.c" "src/shader.c")

# On windows, run GUI application for release and CLI for debug
if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
	target_link_options(d20 PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
ENDIF()

add_library(d20_compiler_flags INTERFACE)
target_compile_features(d20_compiler_flags INTERFACE c_std_11)
if (WIN32)
	set(CMAKE_C_FLAGS_RELEASE "/O2")
else()
	set(CMAKE_C_FLAGS_RELEASE "-O2")
endif()
target_include_directories(d20 PUBLIC ${CMAKE_SOURCE_DIR}/include)

include(FindOpenGL)

target_link_libraries(d20 PUBLIC d20_compiler_flags OpenGL::GL glfw cglm_headers glad freetype)
add_subdirectory(external/glad EXCLUDE_FROM_ALL)
add_subdirectory(external/cglm EXCLUDE_FROM_ALL)
add_subdirectory(external/freetype EXCLUDE_FROM_ALL)
add_subdirectory(external/glfw EXCLUDE_FROM_ALL)


# Copying required data
add_custom_command(
	TARGET d20 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/resources/shaders ${CMAKE_CURRENT_BINARY_DIR}/resources/shaders
	COMMENT "Copying shader files" VERBATIM
)
add_custom_command(
	TARGET d20 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/resources/textures ${CMAKE_CURRENT_BINARY_DIR}/resources/textures
	COMMENT "Copying texture files" VERBATIM
)
add_custom_command(
	TARGET d20 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/resources/fonts ${CMAKE_CURRENT_BINARY_DIR}/resources/fonts
	COMMENT "Copying fonts files" VERBATIM
)