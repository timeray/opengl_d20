/*
* Simple D20 dice roller written using OpenGL.
* 
* Author: Artem Setov
*/
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
#include "animation.h"


const char WINDOW_NAME[] = "D20";


// Control flags
bool g_switch_wire_mode = false;
bool g_start_roll = false;
bool g_is_rolling = false;


typedef struct {
    int width;
    int height;
    const char* name;
} WindowSettings;


typedef struct {
    WindowSettings window;
    SceneSettings scene;
    AnimationSettings anim;
    TextSettings text;
} Settings;


Settings getSettings(void) {
    WindowSettings window_settings = {
        .height = 600.0f,
        .width = 600.0f,
        .name = WINDOW_NAME
    };

    SceneSettings scene_settings = {
        .scale = 0.7f,
        .fov_deg = 45.0f,
        .camera_near_z = 0.1f,
        .camera_far_z = 100.0f,
        .light_direction = { 1.0f, 1.0f, 2.0f },
        .direct_brightness = 1.0f,
        .specular_brightness = 0.5f,
        .ambient_brightness = 0.2f,
        .camera_position = { 0.0f, 0.0f, -5.0f },
    };

    AnimationSettings roll_anim_settings = {
        .idle_rot_speed = 50.0f,

        .n_rotations = 5,
        .n_points = 50,
        .max_rot_speed = 450.0f,
        .min_rot_speed = 100.0f,
        .deaceleration = 150.0f,
    };

    TextSettings text_settings = {
        .text_color = { 0.5f, 0.1f, 0.8f },
        .text_size = 0.5f,
    };

    return (Settings) {
        .window = window_settings,
        .scene = scene_settings,
        .anim = roll_anim_settings,
        .text = text_settings,
    };
}

/* Callbacks */

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


/* Setup */

void setUpOpenGL(GLFWwindow* window) {
    // Initialize glad with current context
    gladLoadGL(glfwGetProcAddress);    

    // Debug settings
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(messageCallback, 0);

    // Graphics settings
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


Status initGLFW(const WindowSettings* settings, GLFWwindow** out_window) {
    puts("Initialize GLFW");
    Status status = STATUS_OK;

    if (!glfwInit()) {
        puts("Unable to initialize GLFW");
        return STATUS_ERR;
    }

    glfwSetErrorCallback(errorCallback);
    GLFWwindow* window = glfwCreateWindow(settings->width, settings->height,
                                          settings->name, NULL, NULL);
    if (!window) {
        puts("Unable to initialize window");
        glfwTerminate();
        return STATUS_ERR;
    }
    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, resizeCallback);
    glfwMakeContextCurrent(window);

    *out_window = window;

    return STATUS_OK;
}


void freeGLFW(GLFWwindow* window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}


/* Rendering */

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


// Main render loop
void renderLoop(GLFWwindow* window, Settings settings, SceneRenderer* scene_renderer_ptr,
                TextRenderer* text_renderer_ptr) {
    double prev_time = glfwGetTime();

    bool is_in_wire_mode = false;
    bool is_in_idle_animation = true;

    versor rot_quat;
    RollAnimationState roll_anim_state = initRollAnimationState(settings.anim.n_points);
    resetRollAnimationState(&roll_anim_state);

    while (!glfwWindowShouldClose(window)) {
        // Clear buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (g_switch_wire_mode) {
            g_switch_wire_mode = false;
            is_in_wire_mode = !is_in_wire_mode;
        }

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
            // printf("Rolled number: %zu\n", dice_value);
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

        // Rendering
        int win_width, win_height;
        glfwGetWindowSize(window, &win_width, &win_height);
        float aspect_ratio = (float)win_width / (float)win_height;

        renderScene(scene_renderer_ptr, &settings.scene, rot_quat, aspect_ratio, is_in_wire_mode);
        renderText(text_renderer_ptr, "Press Esc to exit", &settings.text,
                   10.0f, 10.0f, win_width, win_height);
        renderText(text_renderer_ptr, "Press L for wire mode", &settings.text, 
                   10.0f, 37.0f, win_width, win_height);
        renderText(text_renderer_ptr, "Press Space to roll", &settings.text, 
                   10.0f, 64.0f, win_width, win_height);

        // Swap front buffer (display) with back buffer (where we render to)
        glfwSwapBuffers(window);

        // Fixed framerate
        //glfwSwapInterval(1);  // swap after at least one screen update (aka vsync)
        glfwSwapInterval(0);  // unlimited fps

        // Communicate with the window system to received events and show that applications hasn't locked up 
        glfwPollEvents();
    }

    // Cleanup
    deleteRollAnimationState(&roll_anim_state);
}


int main(void) {
    Settings settings = getSettings();

    srand(time(NULL));  // for dice rolls

    GLFWwindow* window;
    if (initGLFW(&settings.window, &window) != STATUS_OK) {
        return 1;
    }

    setUpOpenGL(window);

    SceneRenderer scene_renderer;
    if (initSceneRenderer(&scene_renderer) != STATUS_OK) {
        freeGLFW(window);
        return 1;
    }

    TextRenderer text_renderer;
    if (initTextRenderer(&text_renderer) != STATUS_OK) {
        freeSceneRenderer(&scene_renderer);
        freeGLFW(window);
        return 1;
    }

    renderLoop(window, settings, &scene_renderer, &text_renderer);

    freeTextRenderer(&text_renderer);
    freeSceneRenderer(&scene_renderer);
    freeGLFW(window);

    return 0;
}
