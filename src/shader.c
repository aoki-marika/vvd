#include "shader.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "path.h"

void shader_print_log(GLuint shader)
{
    // make sure shader is actually a shader
    assert(glIsShader(shader));

    // get the length of the log
    int length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    // get the log contents
    char log[length];
    glGetShaderInfoLog(shader, length, &length, log);

    // only print the log if there is any content
    if (length > 0)
        printf("%s\n", log);
}

void shader_assert_error(GLuint shader, GLenum pname)
{
    // check the shader iv
    GLint check = GL_FALSE;
    glGetShaderiv(shader, pname, &check);

    // if the check is going to fail then print the log first
    if (check != GL_TRUE)
        shader_print_log(shader);

    // make sure the check was successful
    assert(check == GL_TRUE);
}

void shader_create(GLuint *shader, GLenum type, const char *source_path)
{
    // create the shader
    *shader = glCreateShader(type);

    // get the source path relative to the binary directory
    char relative_source_path[PATH_MAX];
    strcpy(relative_source_path, "shaders/");
    strcat(relative_source_path, source_path);

    // get the full source path
    char full_source_path[PATH_MAX];
    path_get_relative(relative_source_path, full_source_path);

    // open the source file and ensure it exists
    FILE *source_file = fopen(full_source_path, "r");
    assert(source_file);

    // the length of the file
    long int length;

    // the buffer that the file is read into
    char *buffer;

    // get the length of the file
    fseek(source_file, 0, SEEK_END);
    length = ftell(source_file);

    // init the buffer
    buffer = malloc(sizeof(char) * (length + 1));

    // read the file into the buffer
    fseek(source_file, 0, SEEK_SET);
    fread(buffer, length, 1, source_file);

    // close the source file
    fclose(source_file);

    // terminate the buffer
    buffer[length] = '\0';

    // set the shader source and compile
    const GLchar *source = (GLchar *)buffer;
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);

    // free the buffer as it is now loaded into the shader
    free(buffer);

    // make sure there are no compilation errors
    shader_assert_error(*shader, GL_COMPILE_STATUS);
}
