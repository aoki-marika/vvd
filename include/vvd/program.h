#pragma once

#include <GLES2/gl2.h>

typedef struct
{
    GLuint id;
    GLuint vertex_id, fragment_id;
} Program;

// source paths should follow the same patterns of shader_creates source_path
Program *program_create(const char *vertex_source_path, const char *fragment_source_path);
void program_free(Program *program);

GLuint program_get_uniform_id(Program *program, const GLchar *name);
GLuint program_get_attribute_id(Program *program, const GLchar *name);

void program_use(Program *program);
