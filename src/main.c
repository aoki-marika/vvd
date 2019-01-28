#include <GLES2/gl2.h>

#include "screen.h"
#include "shader.h"
#include "program.h"

int main()
{
    // create the screen
    Screen *screen = create_screen(screen);

    // create the shaders and program
    GLuint vertex, fragment, program;

    create_shader(&vertex, GL_VERTEX_SHADER, "/home/pi/projects/vvd/bin/test.vs");
    create_shader(&fragment, GL_FRAGMENT_SHADER, "/home/pi/projects/vvd/bin/test.fs");
    create_program(&program, vertex, fragment);

    // the vertex buffer data to display
    GLfloat vertex_buffer_data[] =
    {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         0.0f,  1.0f, 0.0f,
    };

    // clear the colour buffer
    glClear(GL_COLOR_BUFFER_BIT);

    // use the shader pgoram
    glUseProgram(program);

    // load the vertex buffer data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertex_buffer_data);

    // enable the vertex buffer data
    glEnableVertexAttribArray(0);

    // draw the vertex buffer data as triangles
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // render and swap the framebuffer
    void *buffer = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glReadPixels(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

    // free the framebuffer now that its drawn
    free(buffer);

    // update the screen
    update_screen(screen);

    // rendering kept out of while until the frame limiter is implemented
    // crashes the whole pi if rendering without one
    while (1)
    {
    }

    // free the screen from memory
    free(screen);

    return 0;
}
