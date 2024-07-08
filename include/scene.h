#pragma once

#include <glad/gl.h>
#include <cglm/cglm.h>

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


typedef struct {
    float scale;
    float fov_deg;
    float camera_near_z;
    float camera_far_z;

    vec3 light_direction;
    GLfloat direct_brightness;
    GLfloat specular_brightness;
    GLfloat ambient_brightness;

    vec3 camera_position;
} SceneSettings;


Status initSceneRenderer(SceneRenderer*);
void freeSceneRenderer(SceneRenderer*);

void renderScene(SceneRenderer*, SceneSettings*, versor, float, bool);
