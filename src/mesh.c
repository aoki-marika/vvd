#include "mesh.h"

#include <assert.h>

Mesh *mesh_create()
{
    // create the mesh
    Mesh *mesh = malloc(sizeof(Mesh));

    // create the vertex buffer
    glGenBuffers(1, &mesh->vertex_buffer_id);
    assert(mesh->vertex_buffer_id != 0);

    // return the mesh
    return mesh;
}

void mesh_free(Mesh *mesh)
{
    glDeleteBuffers(1, &mesh->vertex_buffer_id);
    free(mesh);
}

void mesh_set_vertices(Mesh *mesh, const GLfloat *vertices, size_t vertices_size)
{
    // bind the vertex buffer and set its vertices
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer_id);
    glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices, GL_STATIC_DRAW);

    // calculate the number of vertices
    mesh->num_vertices = (vertices_size / sizeof(GLfloat)) / 3;
}

void mesh_set_vertices_quad(Mesh *mesh, GLfloat width, GLfloat height)
{
    // generate the quad
    const GLfloat vertices[] =
    {
        -width,  height,  0.0f, //top left
         width,  height,  0.0f, //top right
         width, -height,  0.0f, //bottom right

         width, -height,  0.0f, //bottom right
        -width, -height,  0.0f, //bottom left
        -width,  height,  0.0f, //top left
    };

    // set the mesh vertices
    mesh_set_vertices(mesh, vertices, sizeof(vertices));
}

void mesh_draw(Mesh *mesh, GLuint vertex_attribute_array_id)
{
    // pass the vertex buffer vertices to the given shader
    glEnableVertexAttribArray(vertex_attribute_array_id);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer_id);
    glVertexAttribPointer(vertex_attribute_array_id, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    // draw the mesh
    glDrawArrays(GL_TRIANGLES, 0, mesh->num_vertices);

    // disable the vertex attribute array
    glDisableVertexAttribArray(vertex_attribute_array_id);
}
