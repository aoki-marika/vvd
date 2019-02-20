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

void mesh_set_vertices_quad(Mesh *mesh, int index, GLfloat width, GLfloat height)
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
    mesh_set_vertices(mesh, index, vertices, sizeof(vertices));
}

void mesh_set_vertices_quad_pos(Mesh *mesh, int index, GLfloat width, GLfloat height, vec3_t position)
{
    // calculate the vertice positions
    mat4_t model = m4_identity();
    vec3_t model_position = m4_mul_pos(model, position);

    // generate the quad
    const GLfloat vertices[] =
    {
        model_position.x,         model_position.y + height,  0.0f, //top left
        model_position.x + width, model_position.y + height,  0.0f, //top right
        model_position.x + width, model_position.y,           0.0f, //bottom right

        model_position.x + width, model_position.y,           0.0f, //bottom right
        model_position.x,         model_position.y,           0.0f, //bottom left
        model_position.x,         model_position.y + height,  0.0f, //top left
    };

    // set the mesh vertices
    mesh_set_vertices(mesh, index, vertices, sizeof(vertices));
}

void mesh_draw(Mesh *mesh, int index, size_t size)
{
    // assert that the index and size are valid
    assert(index >= 0 && index < mesh->max_vertices);
    assert(index + size <= mesh->max_vertices);

    // pass the vertex buffer vertices to the mesh program
    glEnableVertexAttribArray(mesh->attribute_vertex_position_id);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer_id);
    glVertexAttribPointer(mesh->attribute_vertex_position_id, MESH_VERTEX_VALUES, GL_FLOAT, GL_FALSE, 0, NULL);

    // draw the mesh
    glDrawArrays(GL_TRIANGLES, index, size);

    // disable the vertex attribute array
    glDisableVertexAttribArray(mesh->attribute_vertex_position_id);
}

void mesh_draw_all(Mesh *mesh)
{
    mesh_draw(mesh, 0, mesh->max_vertices);
}
