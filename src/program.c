#include "program.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

void print_program_log(GLuint program)
{
    // make sure program is actually a program
    assert(glIsProgram(program));

    // get the length of the log
    int length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

    // get the log contents
    char log[length];
    glGetProgramInfoLog(program, length, &length, log);

    // only print the log if there is any content
    if (length > 0)
        printf("%s", log);
}

void assert_program_error(GLuint program, GLenum pname)
{
    // check the program iv
    GLint check = GL_FALSE;
    glGetProgramiv(program, pname, &check);

    // if the check is going to fail then print the log first
    if (check != GL_TRUE)
        print_program_log(program);

    // make sure the check was successful
    assert(check == GL_TRUE);
}

void create_program(GLuint *program, GLuint vertex_shader, GLuint fragment_shader)
{
    // create the program
    *program = glCreateProgram();

    // attach the vertex and fragment shaders
    glAttachShader(*program, vertex_shader);
    glAttachShader(*program, fragment_shader);

    // link the program
    glLinkProgram(*program);

    // make sure there are no link errors
    assert_program_error(*program, GL_LINK_STATUS);

    // use the program
    glUseProgram(*program);
}
