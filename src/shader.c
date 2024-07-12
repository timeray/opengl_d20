#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "shader.h"


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


static Status compileShader(GLuint shader, const char* path) {
    char* shader_text = NULL;
    Status status = loadShaderText(path, &shader_text);

    if (status != STATUS_OK) {
        printf("Unable to read shader\n");
    } else {
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
        }
    }
    return status;
}


typedef enum {
    VERTEX_SHADER,
    FRAGMENT_SHADER
} ShaderType;


static Status initShader(const char* shader_path, ShaderType shader_type, GLuint* out_shader) {
    GLuint shader = glCreateShader(shader_type == VERTEX_SHADER ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
    Status status = compileShader(shader, shader_path);
    if (status == STATUS_OK) {
        *out_shader = shader;
    }
    return status;
}


static void freeShader(GLuint shader) {
    glDeleteShader(shader);
}


Status initProgram(const char* vertex_shader_path, const char* fragment_shader_path, ShaderProgram* shader_program) {
    GLuint vertex_shader, fragment_shader;
    Status status = initShader(vertex_shader_path, VERTEX_SHADER, &vertex_shader);

    if (status == STATUS_OK) {
        status = initShader(fragment_shader_path, FRAGMENT_SHADER, &fragment_shader);
    } else {
        return status;
    }

    if (status == STATUS_ERR) {
        freeShader(vertex_shader);
        return status;
    }

    shader_program->id = glCreateProgram();
    glAttachShader(shader_program->id, vertex_shader);
    glAttachShader(shader_program->id, fragment_shader);
    glLinkProgram(shader_program->id);
    GLint link_success;
    glGetProgramiv(shader_program->id, GL_LINK_STATUS, &link_success);
    if (!link_success) {
        status = STATUS_ERR;
        puts("Shader linking error");
    }
    freeShader(vertex_shader);
    freeShader(fragment_shader);

    return status;
}


void freeProgram(ShaderProgram* shader_program) {
    glDeleteProgram(shader_program->id);
}


GLuint initUniformVariable(GLuint program, const char* name) {
    GLuint index = glGetUniformLocation(program, name);
    if (index == -1) {
        printf("Unable to get uniform variable with name: %s\n", name);
        abort();
    }
    return index;
}
