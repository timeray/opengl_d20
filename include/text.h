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
} TextRenderer;


typedef struct {
    vec3 text_color;
    float text_size;
} TextSettings;


Status initTextRenderer(TextRenderer*);
void freeTextRenderer(TextRenderer*);

void renderText(TextRenderer*, const char*, TextSettings*, float, float, float, float);
