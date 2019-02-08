#pragma once

#include <stdlib.h>
#include <GLES2/gl2.h>

typedef struct
{
    GLuint vertex_buffer_id;
    GLuint num_vertices;
} Mesh;

Mesh *mesh_create();
void mesh_free(Mesh *mesh);

// set the given meshes vertices to the given vertices
// vertices_size should be sizeof(vertices)
void mesh_set_vertices(Mesh *mesh, const GLfloat *vertices, size_t vertices_size);

// a helper method for calling mesh_set_vertices with a quad generated from the given width and height
void mesh_set_vertices_quad(Mesh *mesh, GLfloat width, GLfloat height);

// draw the given mesh with the given vertex shader attribute as the vertex attribute array
void mesh_draw(Mesh *mesh, GLuint vertex_attribute_array_id);
