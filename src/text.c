#include <ft2build.h>
#include FT_FREETYPE_H

#include "text.h"


static const char VERTEX_SHADER_PATH[] = "resources/shaders/text_vertex_shader.glsl";
static const char FRAGMENT_SHADER_PATH[] = "resources/shaders/text_fragment_shader.glsl";
const char TEXT_FONT_PATH[] = "resources/fonts/arial.ttf";


static Status initVertexArray(GLuint* vao_ptr, GLuint* vbo_ptr) {
    const size_t text_n_vertices = 6;
    const size_t text_vertex_size = 4;

    // Create buffers and upload values
    glCreateBuffers(1, vbo_ptr);

    // We will be changing text buffer during rendering
    glNamedBufferData(*vbo_ptr, text_n_vertices * text_vertex_size * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

    glCreateVertexArrays(1, vao_ptr);  // create vertex array objects for dice and text

    glVertexArrayVertexBuffer(*vao_ptr, 0, *vbo_ptr, 0, text_vertex_size * sizeof(GLfloat));

    // Only one attrib for text
    glEnableVertexArrayAttrib(*vao_ptr, 0);
    glVertexArrayAttribFormat(*vao_ptr, 0, text_vertex_size, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(*vao_ptr, 0, 0);

    return STATUS_OK;
}


static void freeVertexArray(GLuint* vao_ptr, GLuint* vbo_ptr) {
    glDeleteBuffers(1, vbo_ptr);
    glDeleteVertexArrays(1, vao_ptr);
}


enum {
    TEXT_N_CHARACTERS = 128
};


static Character g_characters[TEXT_N_CHARACTERS];


static void initCharacters(FT_Face face) {
    // From learnopengl.com
    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    for (unsigned char c = 0; c < TEXT_N_CHARACTERS; ++c) {
        // load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            printf("Failed to load glyph %c\n", c);
            continue;
        }

        GLuint texture;
        glCreateTextures(GL_TEXTURE_2D, 1, &texture);
        if (c != 32) {  // whitespace
            glTextureStorage2D(
                texture, 1, GL_R8,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows
            );
            glTextureSubImage2D(
                texture,
                0, 0, 0,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );

            // set texture options
            glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        g_characters[c].texture_id = texture;
        g_characters[c].size[0] = face->glyph->bitmap.width;
        g_characters[c].size[1] = face->glyph->bitmap.rows;
        g_characters[c].bearing[0] = face->glyph->bitmap_left;
        g_characters[c].bearing[1] = face->glyph->bitmap_top;
        g_characters[c].advance = face->glyph->advance.x;
    }
}


static Status initTextLibrary(void) {
    FT_Library ft_lib;
    FT_Face ft_face;

    Status status = STATUS_OK;
    if (!FT_Init_FreeType(&ft_lib)) {
        if (!FT_New_Face(ft_lib, TEXT_FONT_PATH, 0, &ft_face)) {
            initCharacters(ft_face);

            FT_Done_Face(ft_face);
            FT_Done_FreeType(ft_lib);
        } else {
            puts("Unable to initialize freetype face");
            status = STATUS_ERR;
            FT_Done_FreeType(ft_lib);
        }
    } else {
        puts("Unable to initialize freetype library");
        status = STATUS_ERR;
    }

    return status;
}


static void initUniformVariables(GLuint program, TextUniformVariables* uvars) {
    uvars->color_id = initUniformVariable(program, "textColor");
    uvars->projection_id = initUniformVariable(program, "projection");
}


Status initTextRenderer(TextRenderer* text) {
    Status status = initTextLibrary();
    if (status != STATUS_OK) {
        return status;
    }

    text->char_array_ptr = g_characters;

    status = initVertexArray(&text->vao, &text->vbo);
    if (status != STATUS_OK) {
        puts("Unable to initialize text vertex array");
        return status;
    }

    status = initProgram(VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH, &text->shader);
    if (status != STATUS_OK) {
        puts("Unable to initalize text shader program");
        freeVertexArray(&text->vao, &text->vbo);
        return status;
    }

    initUniformVariables(text->shader.id, &text->uvars);
    return status;
}


void freeTextRenderer(TextRenderer* text) {
    freeVertexArray(&text->vao, &text->vbo);
}


/* Rendering */
static void setTextUniformMatrices(TextUniformVariables* uvars_ptr, vec3 color, mat4 projection) {
    glUniform3f(uvars_ptr->color_id, color[0], color[1], color[2]);
    glUniformMatrix4fv(uvars_ptr->projection_id, 1, GL_FALSE, (float*)projection);
}


static void computeTextGeometry(float width, float height, mat4 projection) {
    glm_ortho(0.0f, width, 0.0f, height, 0.0f, 1.0f, projection);
}


void renderText(TextRenderer* renderer_ptr, const char* text, TextSettings* settings_ptr,
                float x, float y, float window_width, float window_height) {
    glUseProgram(renderer_ptr->shader.id);
    mat4 text_projection;
    computeTextGeometry(window_width, window_height, text_projection);
    setTextUniformMatrices(&renderer_ptr->uvars, settings_ptr->text_color, text_projection);

    float size = settings_ptr->text_size;
    for (size_t i = 0; i < strlen(text); ++i) {

        Character ch = renderer_ptr->char_array_ptr[text[i]];

        GLfloat xpos = x + ch.bearing[0] * size;
        GLfloat ypos = y - (ch.size[1] - ch.bearing[1]) * size;

        GLfloat w = ch.size[0] * size;
        GLfloat h = ch.size[1] * size;
        // update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindVertexArray(renderer_ptr->vao);
        glBindTextureUnit(0, ch.texture_id);
        glNamedBufferSubData(renderer_ptr->vbo, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        // bitshift by 6 to get value in pixels (2^6 = 64)
        x += (ch.advance >> 6) * size;
    }
}