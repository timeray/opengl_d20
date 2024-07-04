#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#include "status.h"
#include "scene.h"
#include "text.h"
#include "icosahedron.h"
#include "animation.h"


const char WINDOW_NAME[] = "D20";


// Control flags
bool g_switch_wire_mode = false;
bool g_start_roll = false;
bool g_is_rolling = false;


void errorCallback(int error, const char* descr) {
    fprintf(stderr, "Error: %s\n", descr);
}


void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        g_switch_wire_mode = true;
    } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && !g_is_rolling) {
        g_start_roll = true;
    }
}


static void resizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}


// OpenGL error callback
void GLAPIENTRY messageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                GLsizei length, const GLchar* message, const void* userParam) {
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
}


/* RENDER */
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

    AnimationSettings anim;

    vec3 text_color;
    float text_size;
} Settings;


void computeDiceGeometry(Settings* settings_ptr, versor rotation_quat, float aspect_ratio,
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


void computeLightingGeometry(mat4 scene_view, vec3 scene_direction, vec3 out_direction) {
    mat3 view_matrix3;
    glm_mat4_pick3(scene_view, view_matrix3);
    vec3 norm_light_direction;
    glm_vec3_copy(scene_direction, norm_light_direction);
    glm_normalize(norm_light_direction);
    glm_mat3_mulv(view_matrix3, norm_light_direction, out_direction);
}


void computeTextGeometry(Settings* settings_ptr, float width, float height, mat4 projection) {
    glm_ortho(0.0f, width, 0.0f, height, 0.0f, 1.0f, projection);
}


void setDiceUniformMatrices(SceneUniformVariables* uvars_ptr,
                            mat4 model, mat4 view, mat3 normal_matrix, mat4 projection) {
    glUniformMatrix4fv(uvars_ptr->model_id, 1, GL_FALSE, (float*)model);
    glUniformMatrix4fv(uvars_ptr->view_id, 1, GL_FALSE, (float*)view);
    glUniformMatrix3fv(uvars_ptr->normal_matrix_id, 1, GL_FALSE, (float*)normal_matrix);
    glUniformMatrix4fv(uvars_ptr->projection_id, 1, GL_FALSE, (float*)projection);
}


void setLightingUniformMatrices(Settings* settings_ptr, SceneUniformVariables* uvars_ptr, vec3 view_direction) {
    glUniform3fv(uvars_ptr->light_dir_id, 1, (float*)view_direction);
    glUniform1f(uvars_ptr->ambient_brightness_id, settings_ptr->ambient_brightness);
    glUniform1f(uvars_ptr->direct_brightness_id, settings_ptr->direct_brightness);
    glUniform1f(uvars_ptr->specular_brightness_id, settings_ptr->specular_brightness);
}


void setTextUniformMatrices(TextUniformVariables* uvars_ptr, vec3 color, mat4 projection) {
    glUniform3f(uvars_ptr->color_id, color[0], color[1], color[2]);
    glUniformMatrix4fv(uvars_ptr->projection_id, 1, GL_FALSE, (float*)projection);
}


void showFpsInWindowTitle(GLFWwindow* window) {
    static double last_time = 0.0;
    static size_t n_frames = 0;

    double current_time = glfwGetTime();
    double delta = current_time - last_time;
    ++n_frames;
    if (delta > 0.5) {
        double fps = n_frames / delta;
        char buf[200];
        sprintf(buf, "%s | %.2f FPS", WINDOW_NAME, fps);
        glfwSetWindowTitle(window, buf);

        n_frames = 0;
        last_time = current_time;
    }
}


void renderDice(Dice* dice_ptr, Settings* settings_ptr, versor rot_quat, float aspect_ratio, bool wireMode) {
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
    } else {
        for (size_t i = 0; i < n / 3; ++i) {
            glDrawArrays(GL_LINE_LOOP, i * 3, 3);
        }
    }
}


void renderText(Text* text_ptr, Settings* settings_ptr, const char* text, float x, float y,
                float window_width, float window_height) {
    glUseProgram(text_ptr->shader.id);
    mat4 text_projection;
    computeTextGeometry(settings_ptr, window_width, window_height, text_projection);
    setTextUniformMatrices(&text_ptr->uvars, settings_ptr->text_color, text_projection);

    glBindVertexArray(text_ptr->vao);

    for (size_t i = 0; i < strlen(text); ++i) {

        Character ch = text_ptr->char_array_ptr[text[i]];

        GLfloat xpos = x + ch.bearing[0] * settings_ptr->text_size;
        GLfloat ypos = y - (ch.size[1] - ch.bearing[1]) * settings_ptr->text_size;

        GLfloat w = ch.size[0] * settings_ptr->text_size;
        GLfloat h = ch.size[1] * settings_ptr->text_size;
        // update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        for (size_t i = 0; i < 6; ++i) {
            vec4 v;
            v[0] = vertices[i][0];
            v[1] = vertices[i][1];
            v[2] = 0.0f;
            v[3] = 1.0f;
            glm_mat4_mulv(text_projection, v, v);
        }
        glBindTextureUnit(0, ch.texture_id);
        glNamedBufferSubData(text_ptr->vbo, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        // bitshift by 6 to get value in pixels (2^6 = 64)
        x += (ch.advance >> 6) * settings_ptr->text_size;
    }
}


void renderLoop(GLFWwindow* window, Settings settings, Dice* dice_ptr, Text* text_ptr) {
    double prev_time = glfwGetTime();

    bool wire_mode = false;
    bool is_in_idle_animation = true;

    versor rot_quat;
    RollAnimationState roll_anim_state = initRollAnimationState(settings.anim.n_points);
    resetRollAnimationState(&roll_anim_state);

    while (!glfwWindowShouldClose(window)) {
        // Clear buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Process flags for the iteration
        if (g_switch_wire_mode) {
            g_switch_wire_mode = false;
            wire_mode = !wire_mode;
        }

        // Logging
        showFpsInWindowTitle(window);

        // Advance time
        double cur_time = glfwGetTime();
        double delta = cur_time - prev_time;
        prev_time = cur_time;

        // Whether to start a new roll
        if (g_start_roll) {
            is_in_idle_animation = false;
            g_start_roll = false;
            g_is_rolling = true;
            resetRollAnimationState(&roll_anim_state);
            glm_quat_copy(rot_quat, roll_anim_state.q_prev);

            size_t dice_value = (size_t)(rand() % 20 + 1);
            printf("Rolled number: %zu\n", dice_value);
            fillRollAnimationQueue(&roll_anim_state, &settings.anim, dice_value);
        }

        // Animation
        if (is_in_idle_animation) {
            // Idle animation
            getIdleAnimationQuaternion(delta, settings.anim.idle_rot_speed, rot_quat);
        } else {
            getRollAnimationQuaternion(delta, &settings.anim, &roll_anim_state, rot_quat);

            // After a roll, enable rolling
            if (g_is_rolling && roll_anim_state.hasFinished) {
                g_is_rolling = false;
            }
        }

        int win_width, win_height;
        glfwGetWindowSize(window, &win_width, &win_height);
        float aspect_ratio = (float)win_width / (float)win_height;

        renderDice(dice_ptr, &settings, rot_quat, aspect_ratio, wire_mode);
        renderText(text_ptr, &settings, "Press Esc to exit", 10.0f, 10.0f, win_width, win_height);
        renderText(text_ptr, &settings, "Press Space to roll", 10.0f, 37.0f, win_width, win_height);

        // Swap front buffer (display) with back buffer (where we render to)
        glfwSwapBuffers(window);

        // Fixed framerate
        //glfwSwapInterval(1);  // swap after at least one screen update (aka vsync)
        glfwSwapInterval(0);  // unlimited fps

        // Communicate with the window system to received events and show that applications hasn't locked up 
        glfwPollEvents();
    }

    deleteRollAnimationState(&roll_anim_state);
}


int main(void) {
    AnimationSettings roll_anim_settings = {
        .idle_rot_speed = 50.0f,

        .n_rotations = 5,
        .n_points = 50,
        .max_rot_speed = 450.0f,
        .min_rot_speed = 100.0f,
        .deaceleration = 150.0f,
    };

    Settings settings = {
        .scale = 0.7f,
        .fov_deg = 45.0f,
        .camera_near_z = 0.1f,
        .camera_far_z = 100.0f,
        .light_direction = { 1.0f, 1.0f, 2.0f },
        .direct_brightness = 1.0f,
        .specular_brightness = 0.5f,
        .ambient_brightness = 0.2f,
        .camera_position = { 0.0f, 0.0f, -5.0f },
        .anim = roll_anim_settings,

        .text_color = { 0.5f, 0.1f, 0.8f },
        .text_size = 0.5f,
    };

    initIcosahedronMeshFromVertices();

    srand(time(NULL));

    puts("Initialize GLFW");

    if (glfwInit()) {
        glfwSetErrorCallback(errorCallback);
        GLFWwindow* window = glfwCreateWindow(600, 600, WINDOW_NAME, NULL, NULL);
        if (window) {
            glfwSetKeyCallback(window, keyCallback);

            glfwMakeContextCurrent(window);

            // Initialize glad with current context
            gladLoadGL(glfwGetProcAddress);

            glfwSetFramebufferSizeCallback(window, resizeCallback);

            // Debug settings
            glEnable(GL_DEBUG_OUTPUT);
            glDebugMessageCallback(messageCallback, 0);

            // Graphics settings
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Allocate buffers and vertex arrays (buffer layouts) to store vertex data 
            // (and not send this data on every render)
            // Vertex array and buffer for dice and text
            Dice dice;
            Text text;
            if (createDice(&dice) == STATUS_OK) {
                if (initText(&text) == STATUS_OK) {
                    renderLoop(window, settings, &dice, &text);
                    freeText(&text);
                }
                freeDice(&dice);
            }

            glfwDestroyWindow(window);
        } else {
            puts("Unable to initialize window");
        }

        puts("Terminate GLFW");
        glfwTerminate();

    } else {
        puts("Unable to initialize GLFW");
    }
}
