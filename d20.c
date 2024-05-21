#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "icosahedron.h"


const char VERTEX_SHADER_PATH[] = "shaders/vertex_shader.glsl";
const char FRAGMENT_SHADER_PATH[] = "shaders/fragment_shader.glsl";
const char TEXTURE_PATH[] = "textures/d20_uv.png";
const char WINDOW_NAME[] = "D20";


// Control flags
bool g_switch_wire_mode = false;
bool g_start_roll = false;
bool g_is_rolling = false;


typedef enum {
    STATUS_OK = 1,
    STATUS_ERR = 0
} Status;


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


void initVertexArrays(GLuint* vao_ptr, GLuint* vbo_ptr) {
    // Vertex array object (vao) and vertex buffer object (vbo)
    // can store multiple arrays and buffers, but we will use only one for the dice

    GLuint loc_attr = 0, col_attr = 1, norm_attr = 2, texture_attr = 3;
    glCreateVertexArrays(1, vao_ptr);  // create one vertex array object    

    // Enable attributes of vertex array
    glEnableVertexArrayAttrib(*vao_ptr, loc_attr);
    glEnableVertexArrayAttrib(*vao_ptr, col_attr);
    glEnableVertexArrayAttrib(*vao_ptr, norm_attr);
    glEnableVertexArrayAttrib(*vao_ptr, texture_attr);

    // Bind attributes to the first (and only) vertex array
    glVertexArrayAttribBinding(*vao_ptr, loc_attr, 0);
    glVertexArrayAttribBinding(*vao_ptr, col_attr, 0);
    glVertexArrayAttribBinding(*vao_ptr, norm_attr, 0);
    glVertexArrayAttribBinding(*vao_ptr, texture_attr, 0);

    // Specify layout (format) for attributes
    const size_t attr_size = 3;
    glVertexArrayAttribFormat(*vao_ptr, loc_attr, attr_size, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(*vao_ptr, col_attr, attr_size, GL_FLOAT, GL_FALSE, attr_size * sizeof(GL_FLOAT));
    glVertexArrayAttribFormat(*vao_ptr, norm_attr, attr_size, GL_FLOAT, GL_FALSE, 2 * attr_size * sizeof(GL_FLOAT));
    glVertexArrayAttribFormat(*vao_ptr, texture_attr, attr_size - 1, GL_FLOAT, GL_FALSE, 3 * attr_size * sizeof(GL_FLOAT));

    // Create buffer and upload values
    glCreateBuffers(1, vbo_ptr);
    glNamedBufferStorage(*vbo_ptr, sizeof(gIcosahedronMesh), &gIcosahedronMesh, 0);

    // Bind array to the buffer
    glVertexArrayVertexBuffer(*vao_ptr, 0, *vbo_ptr, 0, sizeof(Vertex));
}


void freeVertexArrays(GLuint* vao_ptr, GLuint* vbo_ptr) {
    glDeleteBuffers(1, vbo_ptr);
    glDeleteVertexArrays(1, vao_ptr);
}


Status initTextures(const char* path, GLuint* texture_id) {
    int width, height, n_channels;
    unsigned char* data = stbi_load(path, &width, &height, &n_channels, 3);    
    Status status = STATUS_OK;

    if (data) {
        printf("N CHAN = %d\n", n_channels);
        glGenTextures(1, texture_id);
        glBindTexture(GL_TEXTURE_2D, *texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        status = STATUS_ERR;
    }
    stbi_image_free(data);
    return status;
}


void freeTextures(GLuint* texture_id) {
    glDeleteTextures(1, texture_id);
}


/*
* Loads shader code from path to output buffer outBuf
* Caller must free(outBuf).
*/
Status loadShaderText(const char* path, char** outBuf) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        puts("Unable to read file");
        return STATUS_ERR;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buf = malloc(length + 1);
    if (buf) {
        long n_read = fread(buf, 1, length, file);
        if (n_read != length) {
            puts("Error during file read");
            free(buf);
            return STATUS_ERR;
        }

        buf[length] = '\0';
        fclose(file);
    } else {
        puts("Unable to allocate buffer to read file");
        return STATUS_ERR;
    }
    *outBuf = buf;
    return STATUS_OK;
}


void freeShaders(GLuint, GLuint);


Status initShaders(GLuint* vertex_shader_ptr, GLuint* fragment_shader_ptr) {    
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    GLuint shaders[] = { vertex_shader, fragment_shader };
    const char* shader_paths[] = { VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH };
    const char* shader_name[] = { "Vertex shader", "Fragment shader" };
    bool is_shader_compiled[] = { false, false };

    Status status = STATUS_OK;
    
    for (size_t i = 0; i < 2; ++i) {
        GLuint shader = shaders[i];

        char* shader_text = NULL;
        if (loadShaderText(shader_paths[i], &shader_text) != STATUS_OK) {
            printf("Unable to read %s\n", shader_name[i]);
            status = STATUS_ERR;
            break;
        }

#ifdef DEBUG
        printf("%s text:\n%s\n", shader_name[i], shader_text);
#endif

        glShaderSource(shader, 1, &shader_text, NULL);
        glCompileShader(shader);
        free(shader_text);

        // Check for compile errors
        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (!success) {
            GLint max_length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

            // The max_length includes the NULL character
            GLchar* error_log = malloc(max_length);
            if (error_log) {
                glGetShaderInfoLog(shader, max_length, &max_length, &error_log[0]);
                printf("Shader compilation error:\n%s\n", error_log);
            }
            
            status = STATUS_ERR;
        } else {
            is_shader_compiled[i] = true;
        }
    }

    if (status == STATUS_OK) {
        *vertex_shader_ptr = vertex_shader;
        *fragment_shader_ptr = fragment_shader;
    } else {
        // Cleanup
        for (size_t i = 0; i < 2; ++i) {
            if (is_shader_compiled) {
                glDeleteShader(shaders[i]);
            }
        }
    }
    
    return status;
}


void freeShaders(GLuint vertex_shader, GLuint fragment_shader) {
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
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


void render(GLuint* vao_ptr, bool wireMode) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    size_t n = sizeof(gIcosahedronMesh) / sizeof(Vertex);
    if (wireMode == false) {
        for (size_t i = 0; i < 1; ++i) {
            glBindVertexArray(*vao_ptr);
            glDrawArrays(GL_TRIANGLES, 0, n);
        }
    } else {
        for (size_t i = 0; i < n; ++i) {
            glBindVertexArray(*vao_ptr);
            glDrawArrays(GL_LINE_LOOP, i * 3, 3);
        }
    }   
}


typedef struct {
    GLuint model_id;
    GLuint normal_matrix_id;
    GLuint view_id;
    GLuint projection_id;
    GLuint light_dir_id;
    GLuint ambient_brightness_id;
    GLuint direct_brightness_id;
    GLuint specular_brightness_id;
} UniformVariables;


GLuint initUniformVariable(GLuint program, const char* name) {
    GLuint index = glGetUniformLocation(program, name);
    if (index == -1) {
        printf("Unable to get uniform variable with name: %s\n", name);
        abort();
    }
    return index;
}


UniformVariables initUniformVariables(GLuint program) {
    return (UniformVariables) {
        .model_id = initUniformVariable(program, "model"),
        .normal_matrix_id = initUniformVariable(program, "normalMatrix"),
        .view_id = initUniformVariable(program, "view"),
        .projection_id = initUniformVariable(program, "projection"),
        .light_dir_id = initUniformVariable(program, "lightDirection"),
        .ambient_brightness_id = initUniformVariable(program, "ambientBrightness"),
        .direct_brightness_id = initUniformVariable(program, "directBrightness"),
        .specular_brightness_id = initUniformVariable(program, "specularBrightness"),
    };
}


typedef struct {
    float scale;
    float rot_speed_deg;
    float fov_deg;
    float camera_near_z;
    float camera_far_z;

    vec3 light_direction;
    GLfloat direct_brightness;
    GLfloat specular_brightness;
    GLfloat ambient_brightness;

    vec3 camera_position;
} Settings;


void setSceneUniformMatrices(Settings* settings_ptr, UniformVariables* uvars_ptr,
                             versor rotation_quat, float aspect_ratio) {
    // Create Model-View-Projection        
    mat4 model;
    glm_mat4_identity(model);
    glm_scale(model, (vec3) { settings_ptr->scale, settings_ptr->scale, settings_ptr->scale });
    glm_quat_rotate(model, rotation_quat, model);  // apply model rotation for current frame
    glUniformMatrix4fv(uvars_ptr->model_id, 1, GL_FALSE, (float*)model);

    mat4 view;
    glm_mat4_identity(view);
    glm_translate(view, settings_ptr->camera_position);
    glUniformMatrix4fv(uvars_ptr->view_id, 1, GL_FALSE, (float*)view);

    mat4 view_model;
    glm_mat4_mul(view, model, view_model);

    mat4 normal_matrix4;
    glm_mat4_inv(view_model, normal_matrix4);
    glm_mat4_transpose(normal_matrix4);
    mat3 normal_matrix;
    glm_mat4_pick3(normal_matrix4, normal_matrix);
    glUniformMatrix3fv(uvars_ptr->normal_matrix_id, 1, GL_FALSE, (float*)normal_matrix);

    mat4 projection;    
    //glm_ortho_default(aspect_ratio, projection);
    glm_perspective(glm_rad(settings_ptr->fov_deg), aspect_ratio, settings_ptr->camera_near_z,
                    settings_ptr->camera_far_z, projection);
    glUniformMatrix4fv(uvars_ptr->projection_id, 1, GL_FALSE, (float*)projection);

    // Lighting
    mat3 view_matrix3;
    glm_mat4_pick3(view, view_matrix3);
    vec4 view_light_direction = { 0.0f };
    vec3 norm_light_direction;
    glm_vec3_copy(settings_ptr->light_direction, norm_light_direction);;
    glm_normalize(norm_light_direction);
    glm_mat3_mulv(view_matrix3, norm_light_direction, view_light_direction);
    glUniform3fv(uvars_ptr->light_dir_id, 1, (float*)view_light_direction);
    glUniform1f(uvars_ptr->ambient_brightness_id, settings_ptr->ambient_brightness);
    glUniform1f(uvars_ptr->direct_brightness_id, settings_ptr->direct_brightness);
    glUniform1f(uvars_ptr->specular_brightness_id, settings_ptr->specular_brightness);
}


void getDiceRollQuaternion(int dice_value, versor q) {
    int face_idx = getIcosahedronFaceIndex(dice_value);

    size_t face_vertex_idx = face_idx * 3;

    // Rotate face to positive Z direction
    vec3 positive_z_vec = { 0.0f, 0.0f, 1.0f };
    Vertex first_vertex = gIcosahedronMesh[face_vertex_idx];
    vec3 face_normal = { first_vertex.n[0], first_vertex.n[1], first_vertex.n[2] };

    versor q_rot;
    glm_quat_from_vecs(face_normal, positive_z_vec, q_rot);

    // Correct orientation
    vec3 positive_y_vec = { 0.0f, 1.0f, 0.0f };
    Vertex orientation_vertex = gIcosahedronMesh[face_vertex_idx + getOrientationVertexIndex(face_idx)];
    vec3 orient_vec = { orientation_vertex.x, orientation_vertex.y, orientation_vertex.z };
    glm_quat_rotatev(q_rot, orient_vec, orient_vec);
    orient_vec[2] = 0.0f;
    GLfloat orientation_angle = glm_vec3_angle(orient_vec, positive_y_vec);
    if (orient_vec[0] < 0.0f) {
        orientation_angle *= -1;
    }

    // We manually find and specify axis-angle to handle case when orient_vec = -positive_y_vec
    versor q_orient;
    glm_quatv(q_orient, orientation_angle, positive_z_vec);

    // Perform transformations in reverse order
    glm_quat_mul(q_orient, q_rot, q);
}


void getRandomRollQuaternion(versor q) {
    q[0] = (float)rand() / (float)rand();
    q[1] = (float)rand() / (float)rand();
    q[2] = (float)rand() / (float)rand();
    q[3] = 0.0f;
    glm_quat_normalize(q);
}


void getIdleAnimationQuaternion(float time_delta, float rot_speed_deg, versor q_out) {
    static float rot_angle_deg = 0.0f;
    rot_angle_deg += rot_speed_deg * time_delta;

    versor q1, q2, q3;
    glm_quatv(q1, glm_rad(rot_angle_deg), (vec3) { 0.0f, 1.0f, 0.0f });
    glm_quatv(q2, glm_rad(rot_angle_deg * 1.5), (vec3) { 0.0f, 0.0f, 1.0f });
    glm_quatv(q3, glm_rad(rot_angle_deg * 1.75), (vec3) { 1.0f, 0.0f, 0.0f });

    glm_quat_mul(q2, q3, q2);
    glm_quat_mul(q1, q2, q_out);
}


typedef enum {
    ROLL_N_ROTATIONS = 3
};


typedef struct {
    size_t cur_n;
    versor q_arr[ROLL_N_ROTATIONS];
    double t;
    versor q_prev;
    bool hasFinished;
} RollAnimationState;


void resetRollAnimationState(RollAnimationState* state_ptr) {
    state_ptr->cur_n = 0;
    state_ptr->t = 0.0;
    glm_quatv(state_ptr->q_prev, 0.0f, (vec3) { 0.0f, 1.0f, 0.0f });
    state_ptr->hasFinished = false;
}


void fillRollAnimationQueue(RollAnimationState* state_ptr, size_t dice_value) {
    for (size_t n = 0; n < ROLL_N_ROTATIONS - 1; ++n) {
        getRandomRollQuaternion(state_ptr->q_arr[n]);
    }
    getDiceRollQuaternion(dice_value, state_ptr->q_arr[ROLL_N_ROTATIONS - 1]);
}


void getRollAnimationQuaternion(float time_delta, RollAnimationState* state_ptr, versor q_out) {
    // Perform n rotations before moving to final position
    if (state_ptr->cur_n < ROLL_N_ROTATIONS) {
        state_ptr->t += time_delta;  // one rotation per second            

        // Interpolate frame rotation from previous position to desired position
        glm_quat_slerp(state_ptr->q_prev, state_ptr->q_arr[state_ptr->cur_n], glm_min(state_ptr->t, 1.0), q_out);

        if (state_ptr->t >= 1.0) {
            state_ptr->cur_n += 1;
            state_ptr->t = 0.0f;
            glm_quat_copy(q_out, state_ptr->q_prev);
        }
        state_ptr->hasFinished = false;
    } else {
        glm_quat_copy(state_ptr->q_arr[state_ptr->cur_n - 1], q_out);
        state_ptr->hasFinished = true;
    }
}


void renderLoop(GLFWwindow* window, GLuint* vao_ptr, GLuint* tex_ptr, GLuint program,
                Settings settings, UniformVariables uvars) {
    double prev_time = glfwGetTime();

    bool wire_mode = false;
    bool is_in_idle_animation = true;

    versor rot_quat;
    RollAnimationState roll_anim_state;
    resetRollAnimationState(&roll_anim_state);

    while (!glfwWindowShouldClose(window)) {
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
            fillRollAnimationQueue(&roll_anim_state, dice_value);
        }

        // Animation
        if (is_in_idle_animation) {
            // Idle animation
            getIdleAnimationQuaternion(delta, settings.rot_speed_deg, rot_quat);
        } else {
            getRollAnimationQuaternion(delta, &roll_anim_state, rot_quat);

            // After a roll, enable rolling
            if (g_is_rolling && roll_anim_state.hasFinished) {
                g_is_rolling = false;
            }
        }

        // Fill uniform matrices for shaders
        int win_width, win_height;
        glfwGetWindowSize(window, &win_width, &win_height);
        float aspect_ratio = (float)win_width / (float)win_height;
        setSceneUniformMatrices(&settings, &uvars, rot_quat, aspect_ratio);

        // Texture
        glBindTexture(GL_TEXTURE_2D, *tex_ptr);

        // Keep running until close button or Alt+F4        
        render(vao_ptr, wire_mode);

        // Swap front buffer (display) with back buffer (where we render to)
        glfwSwapBuffers(window);

        // Fixed framerate
        //glfwSwapInterval(1);  // swap after at least one screen update (aka vsync)
        glfwSwapInterval(0);  // unlimited fps

        // Communicate with the window system to received events and show that applications hasn't locked up 
        glfwPollEvents();
    }
}


int main(void) {
    Settings settings = {
        .scale = 0.7f,
        .rot_speed_deg = 50.0f,
        .fov_deg = 45.0f,
        .camera_near_z = 0.1f,
        .camera_far_z = 100.0f,
        .light_direction = { 1.0f, 1.0f, 2.0f },
        .direct_brightness = 1.0f,
        .specular_brightness = 0.5f,
        .ambient_brightness = 0.2f,
        .camera_position = { 0.0f, 0.0f, -5.0f }
    };

    initIcosahedronMeshFromVertices();

    srand(42);

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

            // Graphics settings
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);

            // Allocate buffers and vertex arrays (buffer layouts) to store vertex data (and not send this data on every render)
            GLuint vao = 0, vbo = 0;
            initVertexArrays(&vao, &vbo);

            GLuint texture_id = 0;
            if (initTextures(TEXTURE_PATH, &texture_id) == STATUS_OK) {
                GLuint vertex_shader, fragment_shader;
                if (initShaders(&vertex_shader, &fragment_shader)) {
                    GLuint program = glCreateProgram();
                    glAttachShader(program, vertex_shader);
                    glAttachShader(program, fragment_shader);
                    glLinkProgram(program);
                    GLint link_success;
                    glGetProgramiv(program, GL_LINK_STATUS, &link_success);
                    if (link_success) {
                        // We use only one set of shaders, so we can call glUseProgram only once
                        glUseProgram(program);

                        UniformVariables uvars = initUniformVariables(program);

                        renderLoop(window, &vao, &texture_id, program, settings, uvars);
                    } else {
                        puts("Shader program linking error");
                    }

                    // Free stuff
                    glDeleteProgram(program);
                    freeShaders(vertex_shader, fragment_shader);
                } else {
                    puts("Unable to initialize shaders");
                }
            } else {
                puts("Unale to initalize textures");
            }
            freeTextures(&texture_id);

            freeVertexArrays(&vao, &vbo);
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
