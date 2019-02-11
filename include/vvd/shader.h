#pragma once

#include <GLES2/gl2.h>

// creates a shader of the given type with source from the given source path and sets shader to its id
// the source path should be relative to the shaders folder
// program_create should almost always be used instead of this directly
void shader_create(GLuint *shader, GLenum type, const char *source_path);
