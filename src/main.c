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
    Screen *screen = screen_get();

    // setup gles
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // create the shaders and program
    GLuint vertex, fragment, program;

    shader_create(&vertex, GL_VERTEX_SHADER, "/home/pi/projects/vvd/bin/cube.vs");
    shader_create(&fragment, GL_FRAGMENT_SHADER, "/home/pi/projects/vvd/bin/cube.fs");
    program_create(&program, vertex, fragment);

    // get uniform and attribute locations
    GLuint mvp_uniform = glGetUniformLocation(program, "mvp");
    GLuint vertex_position_attribute = glGetAttribLocation(program, "vertexPosition");
    GLuint vertex_colour_attribute = glGetAttribLocation(program, "vertexColour");

    // vertex buffer data for a fullscreen triangle
    const GLfloat vertex_buffer_data[] =
    {
        -1.0f,-1.0f,-1.0f,
        -1.0f,-1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
         1.0f, 1.0f,-1.0f,
        -1.0f,-1.0f,-1.0f,
        -1.0f, 1.0f,-1.0f,
         1.0f,-1.0f, 1.0f,
        -1.0f,-1.0f,-1.0f,
         1.0f,-1.0f,-1.0f,
         1.0f, 1.0f,-1.0f,
         1.0f,-1.0f,-1.0f,
        -1.0f,-1.0f,-1.0f,
        -1.0f,-1.0f,-1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f,-1.0f,
         1.0f,-1.0f, 1.0f,
        -1.0f,-1.0f, 1.0f,
        -1.0f,-1.0f,-1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f,-1.0f, 1.0f,
         1.0f,-1.0f, 1.0f,
         1.0f, 1.0f, 1.0f,
         1.0f,-1.0f,-1.0f,
         1.0f, 1.0f,-1.0f,
         1.0f,-1.0f,-1.0f,
         1.0f, 1.0f, 1.0f,
         1.0f,-1.0f, 1.0f,
         1.0f, 1.0f, 1.0f,
         1.0f, 1.0f,-1.0f,
        -1.0f, 1.0f,-1.0f,
         1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f,-1.0f,
        -1.0f, 1.0f, 1.0f,
         1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
         1.0f,-1.0f, 1.0f
    };

    const GLfloat colour_buffer_data[] =
    {
        0.583f,  0.771f,  0.014f,
        0.609f,  0.115f,  0.436f,
        0.327f,  0.483f,  0.844f,
        0.822f,  0.569f,  0.201f,
        0.435f,  0.602f,  0.223f,
        0.310f,  0.747f,  0.185f,
        0.597f,  0.770f,  0.761f,
        0.559f,  0.436f,  0.730f,
        0.359f,  0.583f,  0.152f,
        0.483f,  0.596f,  0.789f,
        0.559f,  0.861f,  0.639f,
        0.195f,  0.548f,  0.859f,
        0.014f,  0.184f,  0.576f,
        0.771f,  0.328f,  0.970f,
        0.406f,  0.615f,  0.116f,
        0.676f,  0.977f,  0.133f,
        0.971f,  0.572f,  0.833f,
        0.140f,  0.616f,  0.489f,
        0.997f,  0.513f,  0.064f,
        0.945f,  0.719f,  0.592f,
        0.543f,  0.021f,  0.978f,
        0.279f,  0.317f,  0.505f,
        0.167f,  0.620f,  0.077f,
        0.347f,  0.857f,  0.137f,
        0.055f,  0.953f,  0.042f,
        0.714f,  0.505f,  0.345f,
        0.783f,  0.290f,  0.734f,
        0.722f,  0.645f,  0.174f,
        0.302f,  0.455f,  0.848f,
        0.225f,  0.587f,  0.040f,
        0.517f,  0.713f,  0.338f,
        0.053f,  0.959f,  0.120f,
        0.393f,  0.621f,  0.362f,
        0.673f,  0.211f,  0.457f,
        0.820f,  0.883f,  0.371f,
        0.982f,  0.099f,  0.879f
    };

    // setup the vertex buffer
    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

    // setup the colour buffer
    GLuint colour_buffer;
    glGenBuffers(1, &colour_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, colour_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colour_buffer_data), colour_buffer_data, GL_STATIC_DRAW);

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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // use the shader program
        glUseProgram(program);

        // send the mvp matrix to the shader
        glUniformMatrix4fv(mvp_uniform, 1, GL_FALSE, &mvp.m[0][0]);

        // pass the vertex buffer data to the vertex shader
        glEnableVertexAttribArray(vertex_position_attribute);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glVertexAttribPointer(vertex_position_attribute, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        // pass the colour buffer data to the fragment shader
        glEnableVertexAttribArray(vertex_colour_attribute);
        glBindBuffer(GL_ARRAY_BUFFER, colour_buffer);
        glVertexAttribPointer(vertex_colour_attribute, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        // draw the triangle
        glDrawArrays(GL_TRIANGLES, 0, 12 * 3); // 12 triangles, 2 per face

        // disable the vertex attribute array
        glDisableVertexAttribArray(vertex_position_attribute);

        // update the screen
        screen_update(screen);

        // calculte the length of the frame in milliseconds
        frame_end = clock();
        double frame_time = (((double) (frame_end - frame_start)) / CLOCKS_PER_SEC) * 1000.0;

        // sleep for the remainder of the frame if it finished before the framerate limit
        if (frame_time < (1000.0 / SCREEN_RATE))
            usleep((1000.0 / SCREEN_RATE) - frame_time);
    }

    // delete everything
    glDeleteBuffers(1, &vertex_buffer);
    glDeleteBuffers(1, &colour_buffer);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glDeleteProgram(program);

    screen_free(screen);

    return 0;
}
