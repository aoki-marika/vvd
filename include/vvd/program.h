#pragma once

#include <stdbool.h>
#include <GLES2/gl2.h>
#include <arkanis/math_3d.h>

typedef struct
{
    GLuint id;
    GLuint vertex_id, fragment_id;

    // whether or not this programs vertex shader takes projection/view/model matrices
    bool accepts_pvm_matrices;

    // uniforms for the projection, view, and model matrices
    // only used if accepts_pvm_matrices is true
    GLuint uniform_projection_id;
    GLuint uniform_view_id;
    GLuint uniform_model_id;
} Program;

// create a shader program from the given vertex and fragment source paths
// accepts_pvm_matrices should be true if the vertex shader takes projection, view, and model matrices
Program *program_create(const char *vertex_source_path, const char *fragment_source_path, bool accepts_pvm_matrices);
void program_free(Program *program);

// get the id of a uniform in the given programs vertex shader
GLuint program_get_uniform_id(Program *program, const GLchar *name);

// get the id of an attribute in the given programs vertex shader
GLuint program_get_attribute_id(Program *program, const GLchar *name);

// call glUseProgram with the given program
void program_use(Program *program);

// set the projection, view, and model matrices of the given program
// exits if program->accepts_pvm_matrices is false
void program_set_matrices(Program *program, mat4_t projection, mat4_t view, mat4_t model);
