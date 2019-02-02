#pragma once

#include <linux/limits.h>

// gets a path relative to the directory containing the executable and outputs it to output_path
void get_relative_path(const char *path, char output_path[PATH_MAX]);
