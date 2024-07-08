#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "scene.h"
#include "status.h"
#include "icosahedron.h"
#include "shader.h"


static const char VERTEX_SHADER_PATH[] = "resources/shaders/vertex_shader.glsl";
static const char FRAGMENT_SHADER_PATH[] = "resources/shaders/fragment_shader.glsl";
const char TEXTURE_PATH[] = "resources/textures/d20_uv.png";


static Status initVertexArray(GLuint* vao_ptr, GLuint* vbo_ptr) {
    // Create buffers and upload values
    glCreateBuffers(1, vbo_ptr);
    GLuint vbo = *vbo_ptr;

    glNamedBufferStorage(vbo, sizeof(gIcosahedronMesh), &gIcosahedronMesh, 0);

    glCreateVertexArrays(1, vao_ptr);  // create vertex array objects for dice and text
    GLuint vao = *vao_ptr;

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));

    GLuint loc_attr = 0, col_attr = 1, norm_attr = 2, texture_attr = 3;
    // Enable attributes of vertex array
    glEnableVertexArrayAttrib(vao, loc_attr);
    glEnableVertexArrayAttrib(vao, col_attr);
    glEnableVertexArrayAttrib(vao, norm_attr);
    glEnableVertexArrayAttrib(vao, texture_attr);

    // Specify layout (format) for attributes
    glVertexArrayAttribFormat(vao, loc_attr, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, x));
    glVertexArrayAttribFormat(vao, col_attr, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, r));
    glVertexArrayAttribFormat(vao, norm_attr, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, n));
    glVertexArrayAttribFormat(vao, texture_attr, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, t_x));

    // Bind attributes to the first (and only) vertex array
    glVertexArrayAttribBinding(vao, loc_attr, 0);
    glVertexArrayAttribBinding(vao, col_attr, 0);
    glVertexArrayAttribBinding(vao, norm_attr, 0);
    glVertexArrayAttribBinding(vao, texture_attr, 0);

    return STATUS_OK;
}


static void freeVertexArray(GLuint* vao_ptr, GLuint* vbo_ptr) {
    glDeleteBuffers(1, vbo_ptr);
    glDeleteVertexArrays(1, vao_ptr);
}


static Status initTextures(const char* path, GLuint* texture_id) {
    int width, height, n_channels;
    unsigned char* data = stbi_load(path, &width, &height, &n_channels, 3);
    Status status = STATUS_OK;

    if (data) {
        glCreateTextures(GL_TEXTURE_2D, 1, texture_id);
        glTextureParameteri(*texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(*texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(*texture_id, 1, GL_RGB8, width, height);
        glTextureSubImage2D(*texture_id, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateTextureMipmap(*texture_id);
    }
    else {
        status = STATUS_ERR;
    }
    stbi_image_free(data);
    return status;
}


static void freeTextures(GLuint* texture_id) {
    glDeleteTextures(1, texture_id);
}


static void initUniformVariables(GLuint program, SceneUniformVariables* uvars) {
    uvars->model_id = initUniformVariable(program, "model");
    uvars->normal_matrix_id = initUniformVariable(program, "normalMatrix");
    uvars->view_id = initUniformVariable(program, "view");
    uvars->projection_id = initUniformVariable(program, "projection");
    uvars->light_dir_id = initUniformVariable(program, "lightDirection");
    uvars->ambient_brightness_id = initUniformVariable(program, "ambientBrightness");
    uvars->direct_brightness_id = initUniformVariable(program, "directBrightness");
    uvars->specular_brightness_id = initUniformVariable(program, "specularBrightness");
}


Status initSceneRenderer(SceneRenderer* dice) {
    initIcosahedronMeshFromVertices();

    Status status = initVertexArray(&dice->vao, &dice->vbo);
    if (status != STATUS_OK) {
        puts("Unable to initalize vertex array");
        return status;
    }

    status = initTextures(TEXTURE_PATH, &dice->texture);
    if (status != STATUS_OK) {
        puts("Unable to initalize textures");
        freeVertexArray(&dice->vao, &dice->vbo);
        return status;
    }

    status = initProgram(VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH, &dice->shader);
    if (status != STATUS_OK) {
        puts("Unable to initalize shader program");
        freeVertexArray(&dice->vao, &dice->vbo);
        freeTextures(&dice->texture);
        return status;
    }

    initUniformVariables(dice->shader.id, &dice->uvars);
    return status;
}


void freeSceneRenderer(SceneRenderer* dice) {
    freeVertexArray(&dice->vao, &dice->vbo);
    freeTextures(&dice->texture);
    freeProgram(&dice->shader);
}


/* Rendering */
static void setDiceUniformMatrices(SceneUniformVariables* uvars_ptr,
    mat4 model, mat4 view, mat3 normal_matrix, mat4 projection) {
    glUniformMatrix4fv(uvars_ptr->model_id, 1, GL_FALSE, (float*)model);
    glUniformMatrix4fv(uvars_ptr->view_id, 1, GL_FALSE, (float*)view);
    glUniformMatrix3fv(uvars_ptr->normal_matrix_id, 1, GL_FALSE, (float*)normal_matrix);
    glUniformMatrix4fv(uvars_ptr->projection_id, 1, GL_FALSE, (float*)projection);
}


static void setLightingUniformMatrices(
    SceneSettings* settings_ptr, SceneUniformVariables* uvars_ptr, vec3 view_direction
) {
    glUniform3fv(uvars_ptr->light_dir_id, 1, (float*)view_direction);
    glUniform1f(uvars_ptr->ambient_brightness_id, settings_ptr->ambient_brightness);
    glUniform1f(uvars_ptr->direct_brightness_id, settings_ptr->direct_brightness);
    glUniform1f(uvars_ptr->specular_brightness_id, settings_ptr->specular_brightness);
}


static void computeDiceGeometry(SceneSettings* settings_ptr, versor rotation_quat, float aspect_ratio,
                                mat4 model, mat4 view, mat3 normal_matrix, mat4 projection) {
    glm_mat4_identity(model);
    glm_scale(model, (vec3) { settings_ptr->scale, settings_ptr->scale, settings_ptr->scale });
    glm_quat_rotate(model, rotation_quat, model);  // apply model rotation for current frame

    glm_mat4_identity(view);
    glm_translate(view, settings_ptr->camera_position);

    mat4 view_model;
    glm_mat4_mul(view, model, view_model);

    mat4 normal_matrix4;
    glm_mat4_inv(view_model, normal_matrix4);
    glm_mat4_transpose(normal_matrix4);
    glm_mat4_pick3(normal_matrix4, normal_matrix);

    glm_perspective(glm_rad(settings_ptr->fov_deg), aspect_ratio, settings_ptr->camera_near_z,
        settings_ptr->camera_far_z, projection);
}


static void computeLightingGeometry(mat4 scene_view, vec3 scene_direction, vec3 out_direction) {
    mat3 view_matrix3;
    glm_mat4_pick3(scene_view, view_matrix3);
    vec3 norm_light_direction;
    glm_vec3_copy(scene_direction, norm_light_direction);
    glm_normalize(norm_light_direction);
    glm_mat3_mulv(view_matrix3, norm_light_direction, out_direction);
}


void renderScene(SceneRenderer* dice_ptr, SceneSettings* settings_ptr, versor rot_quat,
                float aspect_ratio, bool wireMode) {
    glUseProgram(dice_ptr->shader.id);
    mat4 model, view, projection;
    mat3 normal_matrix;
    computeDiceGeometry(settings_ptr, rot_quat, aspect_ratio, model, view, normal_matrix, projection);
    setDiceUniformMatrices(&dice_ptr->uvars, model, view, normal_matrix, projection);
    vec3 view_light_direction;
    computeLightingGeometry(view, settings_ptr->light_direction, view_light_direction);
    setLightingUniformMatrices(settings_ptr, &dice_ptr->uvars, view_light_direction);

    glBindVertexArray(dice_ptr->vao);
    glBindTextureUnit(0, dice_ptr->texture);
    size_t n = sizeof(gIcosahedronMesh) / sizeof(Vertex);
    if (wireMode == false) {
        glDrawArrays(GL_TRIANGLES, 0, n);
    }
    else {
        for (size_t i = 0; i < n / 3; ++i) {
            glDrawArrays(GL_LINE_LOOP, i * 3, 3);
        }
    }
}
