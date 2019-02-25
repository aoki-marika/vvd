#include "path.h"

#include <unistd.h>
#include <libgen.h>
#include <string.h>

void path_get_relative(const char *path, char output_path[PATH_MAX])
{
    // get the executable path
    char executable_path[PATH_MAX] = { 0 };
    readlink("/proc/self/exe", executable_path, PATH_MAX);

    // get the executable dirname
    char *executable_dirname = dirname(executable_path);

    // ensure executable_dirname will have a "/" dividing it and path
    if (strncmp("/", path, 1) != 0)
        strcat(executable_dirname, "/");

    // join executable_dirname and path in output_path
    strcpy(output_path, executable_dirname);
    strcat(output_path, path);
}
