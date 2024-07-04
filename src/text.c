#include <ft2build.h>
#include FT_FREETYPE_H

#include "text.h"


static const char VERTEX_SHADER_PATH[] = "shaders/text_vertex_shader.glsl";
static const char FRAGMENT_SHADER_PATH[] = "shaders/text_fragment_shader.glsl";
const char TEXT_FONT_PATH[] = "fonts/arial.ttf";


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


Status initText(Text* text) {
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


void freeText(Text* text) {
    freeVertexArray(&text->vao, &text->vbo);
}
