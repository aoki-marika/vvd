#pragma once

#include <GLES2/gl2.h>

// creates a shader of the given type with source from the given source path and sets shader to its id
void shader_create(GLuint *shader, GLenum type, const char *source_path);
