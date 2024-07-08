#pragma once

#include <glad/gl.h>
#include "status.h"


typedef struct {
    GLuint id;
    GLuint vertex_shader;
    GLuint fragment_shader;
} ShaderProgram;


Status initProgram(const char*, const char*, ShaderProgram*);
void freeProgram(ShaderProgram*);

GLuint initUniformVariable(GLuint, const char*);
