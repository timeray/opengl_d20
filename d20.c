#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#define VERTEX_SHADER_PATH "shaders/vertex_shader.glsl"
#define FRAGMENT_SHADER_PATH "shaders/fragment_shader.glsl"
#define WINDOW_NAME "D20"
// Golden ratio (1 + sqrt(5)) / 2
#define GR 1.6180339887498948482045868343656


typedef struct {
    GLfloat x, y, z;
    GLfloat r, g, b;
} Vertex;


GLfloat getDistance(Vertex v1, Vertex v2) {
    return sqrtf(powf(v1.x - v2.x, 2.0) + powf(v1.y - v2.y, 2.0) + powf(v1.z - v2.z, 2.0));
}


bool almostEqual(GLfloat v1, GLfloat v2) {
    if (fabs(v1 - v2) < 0.001) {
        return true;
    } else {
        return false;
    }
}


// Control flags
bool gSwitchWireMode = false;


// Set icosahedron vertices
Vertex gVertices[] = {
    // positions        // colors
    {0.0f,  1.0f,  GR,   1.0f, 0.0f, 0.0f},
    {0.0f,  1.0f, -GR,   0.0f, 1.0f, 0.0f},
    {0.0f, -1.0f,  GR,   0.0f, 0.0f, 1.0f},
    {0.0f, -1.0f, -GR,   1.0f, 0.0f, 1.0f},

    {1.0f,  GR, 0.0f,   1.0f, 1.0f, 0.0f},
    {1.0f, -GR, 0.0f,   0.0f, 0.5f, 0.0f},
    {-1.0f,  GR, 0.0f,   0.0f, 0.5f, 1.0f},
    {-1.0f, -GR, 0.0f,   1.0f, 0.0f, 1.0f},

    {GR, 0.0f,  1.0f,   1.0f, 0.5f, 0.5f},
    {GR, 0.0f, -1.0f,   0.5f, 1.0f, 0.5f},
    {-GR, 0.0f,  1.0f,   0.5f, 0.5f, 1.0f},
    {-GR, 0.0f, -1.0f,   1.0f, 0.0f, 1.0f},
};


Vertex gTriangles[20 * 3];


void generateMeshFromVertices(void) {    
    const size_t n_vertices = sizeof(gVertices) / sizeof(Vertex);
    const size_t n_triangles = sizeof(gTriangles) / sizeof(Vertex);

    size_t count = 0;
    size_t n = 0;
    for (size_t i = 0; i < n_vertices - 2; ++i) {
        for (size_t j = i + 1; j < n_vertices - 1; ++j) {
            for (size_t k = j + 1; k < n_vertices; ++k) {        

                if (!almostEqual(getDistance(gVertices[i], gVertices[j]), 2.0f)) {
                    continue;
                }
                if (!almostEqual(getDistance(gVertices[i], gVertices[k]), 2.0f)) {
                    continue;
                }
                if (!almostEqual(getDistance(gVertices[j], gVertices[k]), 2.0f)) {
                    continue;
                }

                assert(count < n_triangles);

                Vertex p1 = gVertices[i];
                Vertex p2 = gVertices[j];
                Vertex p3 = gVertices[k];

                // Calculate the direction of the normal of the plane relative to origin
                // to get the proper winding order (for face culling)
                vec3 v1 = {p1.x - p2.x, p1.y - p2.y, p1.z - p2.z};
                vec3 v2 = {p1.x - p3.x, p1.y - p3.y, p1.z - p3.z};

                vec3 n;
                glm_vec3_crossn(v1, v2, n);

                // Constant of the plane equation
                float c = p1.x * n[0] + p1.y * n[1] + p1.z * n[2];

                if (c > 0.0f) {
                    gTriangles[count++] = p1;
                    gTriangles[count++] = p2;
                    gTriangles[count++] = p3;
                } else {
                    gTriangles[count++] = p2;
                    gTriangles[count++] = p1;
                    gTriangles[count++] = p3;
                }

                
            }
        }
    }
}


typedef enum {
    STATUS_OK = 1,
    STATUS_ERR = 0
} Status;


void errorCallback(int error, const char* descr) {
    fprintf(stderr, "Error: %s\n", descr);
}


static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        gSwitchWireMode = true;
    }
}


static void resizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}


void initVertexArrays(GLuint* vao_ptr, GLuint* vbo_ptr) {
    // Vertex array object (vao) and vertex buffer object (vbo)
    // can store multiple arrays and buffers, but we will use only one for the dice

    GLuint loc_attr = 0, col_attr = 1;
    glCreateVertexArrays(1, vao_ptr);  // create one vertex array object    

    // Enable attributes of vertex array
    glEnableVertexArrayAttrib(*vao_ptr, loc_attr);
    glEnableVertexArrayAttrib(*vao_ptr, col_attr);

    // Bind attributes to the first (and only) vertex array
    glVertexArrayAttribBinding(*vao_ptr, loc_attr, 0);
    glVertexArrayAttribBinding(*vao_ptr, col_attr, 0);

    // Specify layout (format) for attributes
    glVertexArrayAttribFormat(*vao_ptr, loc_attr, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(*vao_ptr, col_attr, 3, GL_FLOAT, GL_FALSE, 12);

    // Create buffer and upload values
    glCreateBuffers(1, vbo_ptr);
    glNamedBufferStorage(*vbo_ptr, sizeof(gTriangles), &gTriangles, 0);

    // Bind array to the buffer
    glVertexArrayVertexBuffer(*vao_ptr, 0, *vbo_ptr, 0, 24);
}


void freeVertexArrays(GLuint* vao_ptr, GLuint* vbo_ptr) {
    glDeleteBuffers(1, vbo_ptr);
    glDeleteVertexArrays(1, vao_ptr);
}


/*
* Loads shader code from path to output buffer outBuf
* Caller must free(outBuf).
*/
static Status loadShaderText(const char* path, char** outBuf) {
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
    char* shader_paths[] = { VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH };
    char* shader_name[] = { "Vertex shader", "Fragment shader" };
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


static void showFpsInWindowTitle(GLFWwindow* window) {
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

    if (wireMode == false) {
        for (size_t i = 0; i < 1; ++i) {
            glBindVertexArray(*vao_ptr);
            glDrawArrays(GL_TRIANGLES, 0, sizeof(gTriangles) / sizeof(Vertex));
        }
    } else {
        for (size_t i = 0; i < 20; ++i) {
            glBindVertexArray(*vao_ptr);
            glDrawArrays(GL_LINE_LOOP, i * 3, 3);
        }
    }   
}


void renderLoop(GLFWwindow* window, GLuint* vao_ptr, GLuint program) {
    double prev_time = glfwGetTime();
    float rot_speed_deg = 20;
    float scale = 0.5f;
    vec3 scale_vec = { scale, scale, scale };
    bool wireMode = false;

    while (!glfwWindowShouldClose(window)) {
        // Process flags for the iteration
        if (gSwitchWireMode) {
            gSwitchWireMode = false;
            wireMode = !wireMode;
        }

        // logging
        showFpsInWindowTitle(window);

        // Geometry
        // Get transformation matrix ID for shaders
        GLuint transf_location = glGetUniformLocation(program, "transformation");
        assert(transf_location != -1);

        // Create transformation matrix
        double cur_time = glfwGetTime();
        double delta = cur_time - prev_time;

        float rot_angle_deg = rot_speed_deg * delta;

        mat4 model;
        glm_mat4_identity(model);
        glm_scale(model, scale_vec);
        glm_rotate(model, glm_rad(rot_angle_deg), (vec3) { 0.0f, 1.0f, 0.0f });

        mat4 view;
        glm_mat4_identity(view);
        glm_translate(view, (vec3) { 0.0f, 0.0f, -5.0f });        
        //glm_rotate(view, glm_rad(15.0f), (vec3) { 1.0f, 0.0f, 0.0f });
        //glm_rotate(view, glm_rad(15.0f), (vec3) { 0.0f, 1.0f, 0.0f });

        mat4 projection;
        int win_width, win_height;
        glfwGetWindowSize(window, &win_width, &win_height);
        float aspect_ratio = (float)win_width / (float)win_height;
        //glm_ortho_default(aspect_ratio, projection);
        glm_perspective(glm_rad(45.0f), aspect_ratio, 0.1f, 100.0f, projection);

        glm_mat4_mul(projection, view, projection);
        glm_mat4_mul(projection, model, projection);

        glUniformMatrix4fv(transf_location, 1, GL_FALSE, (float*)projection);

        // Keep running until close button or Alt+F4        
        render(vao_ptr, wireMode);

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
    generateMeshFromVertices();

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
            glCullFace(GL_FRONT);

            // Allocate buffers and vertex arrays (buffer layouts) to store vertex data (and not send this data on every render)
            GLuint vao, vbo;
            initVertexArrays(&vao, &vbo);

            GLuint vertex_shader, fragment_shader;
            if (initShaders(&vertex_shader, &fragment_shader)) {
                GLuint program = glCreateProgram();
                glAttachShader(program, vertex_shader);
                glAttachShader(program, fragment_shader);
                glLinkProgram(program);

                // We use only one set of shaders, so we can call glUseProgram only once
                glUseProgram(program);

                renderLoop(window, &vao, program);

                // Free stuff
                glDeleteProgram(program);
                freeShaders(vertex_shader, fragment_shader);
            } else {
                puts("Unable to initialize shaders");
            }

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
