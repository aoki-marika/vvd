#include <time.h>
#include <unistd.h>
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

    // load the vertex buffer data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertex_buffer_data);

    // enable the vertex buffer data
    glEnableVertexAttribArray(0);

    while (1)
    {
        // store the time at the start of the frame to calculate the duration of the frame
        clock_t start_time = clock();

        // clear the colour buffer
        glClear(GL_COLOR_BUFFER_BIT);

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

        // calculte the length of the frame in milliseconds
        double frame_time = (clock() - start_time) / (CLOCKS_PER_SEC / 1000.0);

        // sleep for the remainder of the frame if it finished before the framerate limit
        if (frame_time < (1000.0 / SCREEN_RATE))
            usleep((1000.0 / SCREEN_RATE) - frame_time);
    }

    // free the screen from memory
    free(screen);

    return 0;
}
