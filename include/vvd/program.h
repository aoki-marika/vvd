#pragma once

#include <GLES2/gl2.h>

// create a shader program with the given vertex and fragment shaders and set program to its id
void program_create(GLuint *program, GLuint vertex_shader, GLuint fragment_shader);
