#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sndfile.h>

typedef struct
{
    int id;

    bool preload;
    uint64_t buffer_size;
    float *buffer; //only used if preload is true

    SNDFILE *file;
    SF_INFO info;
} AudioFile;

// if preload is true then the audio file is loaded into memory and audio_file_read reads from memory instead of disk
AudioFile *audio_file_open(const char *path, bool preload);
void audio_file_free(AudioFile *audio_file);

// reads the given length at the given offset from the given audio file
// output is set to the data that was read
// returns the length of what was read
uint64_t audio_file_read(AudioFile *audio_file, uint64_t offset, uint64_t length, float *output);
