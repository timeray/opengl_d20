#pragma once

#include <glad/gl.h>
#include "status.h"
#include "shader.h"


typedef struct {
    GLuint model_id;
    GLuint normal_matrix_id;
    GLuint view_id;
    GLuint projection_id;
    GLuint light_dir_id;
    GLuint ambient_brightness_id;
    GLuint direct_brightness_id;
    GLuint specular_brightness_id;
} SceneUniformVariables;


typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint texture;
    ShaderProgram shader;
    SceneUniformVariables uvars;
} SceneRenderer;


Status createDice(SceneRenderer*);
void freeDice(SceneRenderer*);
