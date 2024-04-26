# Target Start
add_library(glad src/gl.c)

target_include_directories(glad
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(glad INTERFACE d20_compiler_flags)