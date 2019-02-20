#pragma once

#include <stdlib.h>
#include <GLES2/gl2.h>
#include <arkanis/math_3d.h>

#include "program.h"

// the number of values in one vertex (x, y, z)
#define MESH_VERTEX_VALUES 3

// the number of vertices for basic shapes
#define MESH_VERTICES_TRIANGLE 3
#define MESH_VERTICES_QUAD MESH_VERTICES_TRIANGLE * 2

typedef struct
{
    // the id of this meshes vertex buffer
    GLuint vertex_buffer_id;

    // the maximum number of vertices in this meshes vertex buffer
    GLuint max_vertices;

    // the program for this meshes geometry and material
    Program *program;
    GLuint attribute_vertex_position_id;
} Mesh;

// create a new mesh of the given type with the given number of maximum vertices using the given program for its geometry and material
// the given programs vertex shader must have a vec3 vertexPosition attribute
Mesh *mesh_create(int max_vertices, Program *program, GLenum usage);
void mesh_free(Mesh *mesh);

// set the given meshes vertices to the given vertices, offset by the given vertice index
// vertices_size should be sizeof(vertices)
void mesh_set_vertices(Mesh *mesh, int index, const GLfloat *vertices, size_t vertices_size);

// a helper method for calling mesh_set_vertices with a quad generated from the given width and height
void mesh_set_vertices_quad(Mesh *mesh, int index, GLfloat width, GLfloat height);

// a helper method for calling mesh_set_vertices with a quad generated from the given width and height and position
void mesh_set_vertices_quad_pos(Mesh *mesh, int index, GLfloat width, GLfloat height, vec3_t position);

// draw the given meshes vertices starting at the given vertex index and of the given size
void mesh_draw(Mesh *mesh, int index, size_t size);

// draw all of the given meshes vertices
void mesh_draw_all(Mesh *mesh);
