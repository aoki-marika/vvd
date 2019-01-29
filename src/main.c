#include <time.h>
#include <unistd.h>
#include <GLES2/gl2.h>

#define MATH_3D_IMPLEMENTATION
#include <arkanis/math_3d.h>

#include "screen.h"
#include "shader.h"
#include "program.h"

int main()
{
    // create the screen
    Screen *screen = create_screen(screen);

    // create the shaders and program
    GLuint vertex, fragment, program;

    create_shader(&vertex, GL_VERTEX_SHADER, "/home/pi/projects/vvd/bin/triangle.vs");
    create_shader(&fragment, GL_FRAGMENT_SHADER, "/home/pi/projects/vvd/bin/triangle.fs");
    create_program(&program, vertex, fragment);

    // get uniform and attribute locations
    GLuint mvp_uniform = glGetUniformLocation(program, "mvp");
    GLuint vertex_position_attribute = glGetAttribLocation(program, "vertexPosition");

    // vertex buffer data for a fullscreen triangle
    const GLfloat vertex_buffer_data[] =
    {
        -1.0f, -1.0f,  0.0f, // bottom left
         1.0f, -1.0f,  0.0f, // bottom right
         0.0f,  1.0f,  0.0f, // top center
    };

    // setup the vertex buffer
    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

    // create the projection, view, and model matrices
    mat4_t projection = m4_perspective(45.0f,
                                       (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
                                       0.1f,
                                       100.0f);

    mat4_t view = m4_look_at(vec3(4, 3, 3),
                             vec3(0, 0, 0),
                             vec3(0, 1, 0));

    mat4_t model = m4_identity();

    // calculate the mvp matrix
    mat4_t mvp = projection;
    mvp = m4_mul(mvp, view);
    mvp = m4_mul(mvp, model);

    while (1)
    {
        // store the time at the start of the frame to calculate the duration of the frame
        clock_t start_time = clock();

        // clear the colour buffer
        glClear(GL_COLOR_BUFFER_BIT);

        // use the shader program
        glUseProgram(program);

        // send the mvp matrix to the shader
        glUniformMatrix4fv(mvp_uniform, 1, GL_FALSE, &mvp.m[0][0]);

        // pass the vertex buffer data to the vertex shader
        glEnableVertexAttribArray(vertex_position_attribute);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glVertexAttribPointer(vertex_position_attribute, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        // draw the triangle
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // disable the vertex attribute array
        glDisableVertexAttribArray(vertex_position_attribute);

        // update the screen
        update_screen(screen);

        // calculte the length of the frame in milliseconds
        double frame_time = (clock() - start_time) / (CLOCKS_PER_SEC / 1000.0);

        // sleep for the remainder of the frame if it finished before the framerate limit
        if (frame_time < (1000.0 / SCREEN_RATE))
            usleep((1000.0 / SCREEN_RATE) - frame_time);
    }

    // delete everything
    glDeleteBuffers(1, &vertex_buffer);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glDeleteProgram(program);

    free(screen);

    return 0;
}
