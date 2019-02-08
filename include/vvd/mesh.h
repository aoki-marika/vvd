#pragma once

#include <stdlib.h>
#include <GLES2/gl2.h>

#include "program.h"

typedef struct
{
    GLuint vertex_buffer_id;
    GLuint num_vertices;

    Program *program;
    GLuint attribute_vertex_position_id;
} Mesh;

// create a new mesh with the given program for its geometry and material
// the given programs vertex shader must have a vec3 vertexPosition attribute
Mesh *mesh_create(Program *program);
void mesh_free(Mesh *mesh);

// set the given meshes vertices to the given vertices
// vertices_size should be sizeof(vertices)
void mesh_set_vertices(Mesh *mesh, const GLfloat *vertices, size_t vertices_size);

// a helper method for calling mesh_set_vertices with a quad generated from the given width and height
void mesh_set_vertices_quad(Mesh *mesh, GLfloat width, GLfloat height);

// draw the given mesh
void mesh_draw(Mesh *mesh);
