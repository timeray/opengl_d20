#pragma once

#include <glad/gl.h>
#include <cglm/cglm.h>

#include "status.h"
#include "shader.h"


typedef struct {
    GLuint texture_id;
    ivec2 size;
    ivec2 bearing;
    GLuint advance;
} Character;


typedef struct {
    GLuint color_id;
    GLuint projection_id;
} TextUniformVariables;


typedef struct {
    GLuint vao;
    GLuint vbo;
    ShaderProgram shader;
    TextUniformVariables uvars;
    Character* char_array_ptr;
} Text;


Status initText(Text*);
void freeText(Text*);
