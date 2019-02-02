#include "hid_config.h"

#include <stdio.h>
#include <assert.h>

bool hid_config_is_valid(const char *path)
{
    // open the config file for reading
    FILE *file = fopen(path, "r");

    // if the file doesnt exist say it is invalid
    if (!file)
        return false;

    // get the length of the file
    fseek(file, 0, SEEK_END);
    long int length = ftell(file);

    // close the file and return whether the size is correct or not
    fclose(file);
    return length == sizeof(HIDConfig);
}

HIDConfig hid_config_read(const char *path)
{
    // open the config file for reading binary
    FILE *file = fopen(path, "rb");

    // assert that the file is opened
    assert(file);

    // read the file into a config
    HIDConfig config;
    fread(&config, sizeof(HIDConfig), 1, file);

    // close the file
    fclose(file);

    // return the config
    return config;
}

void hid_config_write(HIDConfig config, const char *path)
{
    // open the config file for writing and/or creating binary
    FILE *file = fopen(path, "wb+");

    // assert that the file is opened
    assert(file);

    // write the config to the file
    fwrite(&config, sizeof(HIDConfig), 1, file);

    // close the file
    fclose(file);
}
