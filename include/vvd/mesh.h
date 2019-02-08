#pragma once

#include <stdlib.h>
#include <GLES2/gl2.h>
#include <arkanis/math_3d.h>

#include "program.h"

// number of vertices for basic data types
#define MESH_VERTICES_TRIANGLE 3
#define MESH_VERTICES_QUAD MESH_VERTICES_TRIANGLE * 2

typedef struct
{
    GLuint vertex_buffer_id;
    GLuint num_vertices;

    Program *program;
    GLuint attribute_vertex_position_id;
} Mesh;

// create a new mesh with the given number of vertices using the given program for its geometry and material
// the given programs vertex shader must have a vec3 vertexPosition attribute
Mesh *mesh_create(int num_vertices, Program *program);
void mesh_free(Mesh *mesh);

// set the given meshes vertices to the given vertices, offset by the given vertice index
// vertices_size should be sizeof(vertices)
void mesh_set_vertices(Mesh *mesh, int index, const GLfloat *vertices, size_t vertices_size);

// a helper method for calling mesh_set_vertices with a quad generated from the given width and height
void mesh_set_vertices_quad(Mesh *mesh, int index, GLfloat width, GLfloat height);

// a helper method for calling mesh_set_vertices with a quad generated from the given width and height and position
void mesh_set_vertices_quad_pos(Mesh *mesh, int index, GLfloat width, GLfloat height, vec3_t position);

// draw the given mesh
void mesh_draw(Mesh *mesh);
