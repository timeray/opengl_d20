#pragma once

#include <glad/gl.h>
#include "status.h"


typedef struct {
    GLuint id;  // OpenGL program id
    GLuint vertex_shader;
    GLuint fragment_shader;
} ShaderProgram;


// Initialize shader program
Status initProgram(const char* vertex_shader_path, const char* fragment_shader_path,
                   ShaderProgram* shader_program);
void freeProgram(ShaderProgram* shader_program);

// Initialize uniform variable for OpenGL program
GLuint initUniformVariable(GLuint program, const char* name);
