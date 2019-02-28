#include "mesh.h"

#include <assert.h>

Mesh *mesh_create(int max_vertices, Program *program, GLenum usage)
{
    // create the mesh
    Mesh *mesh = malloc(sizeof(Mesh));
    mesh->max_vertices = max_vertices;
    mesh->program = program;

    // get the vertex position attribute
    mesh->attribute_vertex_position_id = program_get_attribute_id(program, "vertexPosition");

    // create the vertex buffer
    glGenBuffers(1, &mesh->vertex_buffer_id);
    assert(mesh->vertex_buffer_id != 0);

    // allocate the vertex buffer data
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer_id);
    glBufferData(GL_ARRAY_BUFFER, mesh->max_vertices * MESH_VERTEX_VALUES * sizeof(float), NULL, usage);

    // return the mesh
    return mesh;
}

void mesh_free(Mesh *mesh)
{
    glDeleteBuffers(1, &mesh->vertex_buffer_id);
    free(mesh);
}

void mesh_set_vertices(Mesh *mesh, int index, const GLfloat *vertices, size_t vertices_size)
{
    // bind the vertex buffer and set its vertices starting at offset
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer_id);
    glBufferSubData(GL_ARRAY_BUFFER, index * MESH_VERTEX_VALUES * sizeof(float), vertices_size, vertices);
}

void mesh_set_vertices_quad(Mesh *mesh, int index, GLfloat width, GLfloat height, vec3_t position)
{
    // calculate the vertex positions
    vec3_t start_position = vec3(-position.x, position.y, position.z);
    vec3_t end_position = vec3(-position.x - width, position.y + height, position.z);

    // generate the quad
    const GLfloat vertices[] =
    {
        end_position.x,   start_position.y, start_position.z, //top left
        end_position.x,   end_position.y,   start_position.z, //top right
        start_position.x, end_position.y,   start_position.z, //bottom right

        start_position.x, end_position.y,   start_position.z, //bottom right
        start_position.x, start_position.y, start_position.z, //bottom left
        end_position.x,   start_position.y, start_position.z, //top left
    };

    // set the mesh vertices
    mesh_set_vertices(mesh, index, vertices, sizeof(vertices));
}

void mesh_set_vertices_quad_edges(Mesh *mesh, int index, GLfloat width, vec3_t start_position, vec3_t end_position)
{
    // calculate the vertex positions
    vec3_t draw_start_position = vec3(-start_position.x, start_position.y, start_position.z);
    vec3_t draw_end_position = vec3(-end_position.x, end_position.y, end_position.z);

    // generate the quad
    const GLfloat vertices[] =
    {
        draw_end_position.x,           draw_end_position.y,   draw_start_position.z, //top left
        draw_end_position.x - width,   draw_end_position.y,   draw_start_position.z, //top right
        draw_start_position.x - width, draw_start_position.y, draw_start_position.z, //bottom right

        draw_start_position.x - width, draw_start_position.y, draw_start_position.z, //bottom right
        draw_start_position.x,         draw_start_position.y, draw_start_position.z, //bottom left
        draw_end_position.x,           draw_end_position.y,   draw_start_position.z, //top left
    };

    // set the mesh vertices
    mesh_set_vertices(mesh, index, vertices, sizeof(vertices));
}

void mesh_draw_start(Mesh *mesh)
{
    // pass the vertex buffer vertices to the meshes program
    glEnableVertexAttribArray(mesh->attribute_vertex_position_id);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer_id);
    glVertexAttribPointer(mesh->attribute_vertex_position_id, MESH_VERTEX_VALUES, GL_FLOAT, GL_FALSE, 0, NULL);
}

void mesh_draw_end(Mesh *mesh)
{
    // disable the vertex attribute array
    glDisableVertexAttribArray(mesh->attribute_vertex_position_id);
}

void mesh_draw_vertices(Mesh *mesh, int index, size_t size)
{
    // assert that the index and size are valid
    assert(index >= 0 && index < mesh->max_vertices);
    assert(index + size <= mesh->max_vertices);

    // draw the vertices
    glDrawArrays(GL_TRIANGLES, index, size);
}

void mesh_draw_all(Mesh *mesh)
{
    mesh_draw_start(mesh);
    mesh_draw_vertices(mesh, 0, mesh->max_vertices);
    mesh_draw_end(mesh);
}
