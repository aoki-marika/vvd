#include "program.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "shader.h"

void program_print_log(GLuint program)
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
        printf("%s\n", log);
}

void program_assert_error(GLuint program, GLenum pname)
{
    // check the program iv
    GLint check = GL_FALSE;
    glGetProgramiv(program, pname, &check);

    // if the check is going to fail then print the log first
    if (check != GL_TRUE)
        program_print_log(program);

    // make sure the check was successful
    assert(check == GL_TRUE);
}

Program *program_create(const char *vertex_source_path, const char *fragment_source_path)
{
    // create the program
    Program *program = malloc(sizeof(Program));
    program->id = glCreateProgram();

    // create the vertex and fragment shaders
    shader_create(&program->vertex_id, GL_VERTEX_SHADER, vertex_source_path);
    shader_create(&program->fragment_id, GL_FRAGMENT_SHADER, fragment_source_path);

    // attach the vertex and fragment shaders
    glAttachShader(program->id, program->vertex_id);
    glAttachShader(program->id, program->fragment_id);

    // link the program
    glLinkProgram(program->id);

    // make sure there are no link errors
    program_assert_error(program->id, GL_LINK_STATUS);

    // use the program
    glUseProgram(program->id);

    // return the program
    return program;
}

void program_free(Program *program)
{
    glDeleteShader(program->vertex_id);
    glDeleteShader(program->fragment_id);
    glDeleteProgram(program->id);
    free(program);
}

GLuint program_get_uniform_id(Program *program, const GLchar *name)
{
    return glGetUniformLocation(program->id, name);
}

GLuint program_get_attribute_id(Program *program, const GLchar *name)
{
    return glGetAttribLocation(program->id, name);
}

void program_use(Program *program)
{
    glUseProgram(program->id);
}
