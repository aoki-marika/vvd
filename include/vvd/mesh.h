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

// a helper method for calling mesh_set_vertices with a quad generated from the given width and height and position
// origin point is bottom-left
// -x = left, +x = right
// -y = down, +y = up
void mesh_set_vertices_quad(Mesh *mesh, int index, GLfloat width, GLfloat height, vec3_t position);

// a helper method for calling mesh_set_vertices with a quad generated from the given width, drawn from the given start to end positions
// origin and position are the same as mesh_set_vertices_quad
void mesh_set_vertices_quad_edges(Mesh *mesh, int index, GLfloat width, vec3_t start_position, vec3_t end_position);

// bind the given mesh so it can draw vertices
// this function only binds the given meshes vertex buffer, the program needs to be used manually
void mesh_draw_start(Mesh *mesh);

// unbind the given mesh after caling mesh_draw_start
void mesh_draw_end(Mesh *mesh);

// draw the vertices of the given mesh at the given index of the given size
// must call mesh_draw_start first
void mesh_draw_vertices(Mesh *mesh, int index, size_t size);

// draw all of the given meshes vertices
// automatically calls mesh_draw_start and mesh_draw_end
void mesh_draw_all(Mesh *mesh);
