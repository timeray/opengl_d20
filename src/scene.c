#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "scene.h"
#include "status.h"
#include "icosahedron.h"
#include "shader.h"


static const char VERTEX_SHADER_PATH[] = "shaders/vertex_shader.glsl";
static const char FRAGMENT_SHADER_PATH[] = "shaders/fragment_shader.glsl";
const char TEXTURE_PATH[] = "textures/d20_uv.png";


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


Status createDice(Dice* dice) {
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


void freeDice(Dice* dice) {
    freeVertexArray(&dice->vao, &dice->vbo);
    freeTextures(&dice->texture);
    freeProgram(&dice->shader);
}