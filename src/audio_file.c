#include "audio_file.h"

#include <stdio.h>
#include <stdlib.h>

AudioFile *audio_file_open(const char *path, bool preload)
{
    AudioFile *audio_file = malloc(sizeof(AudioFile));
    audio_file->preload = preload;

    // open the audio file
    if ((audio_file->file = sf_open(path, SFM_READ, &audio_file->info)) == NULL)
    {
        fprintf(stderr, "Unable to open audio file %s: %s\n", path, sf_strerror(NULL));
        exit(1);
    }

    // preload the file if its preloaded
    if (preload)
    {
        audio_file->buffer_size = audio_file->info.frames * audio_file->info.channels;
        audio_file->buffer = malloc(audio_file->buffer_size * sizeof(float));
        sf_read_float(audio_file->file, audio_file->buffer, audio_file->buffer_size);
    }

    return audio_file;
}

void audio_file_free(AudioFile *audio_file)
{
    if (audio_file->buffer)
        free(audio_file->buffer);

    sf_close(audio_file->file);
    free(audio_file);
}

uint64_t audio_file_read(AudioFile *audio_file, uint64_t offset, uint64_t length, float *output)
{
    if (audio_file->preload)
    {
        // the length of what to read
        uint64_t read_length = offset + length > audio_file->buffer_size ? audio_file->buffer_size - offset : length;

        // put the audio data into output
        for (uint64_t i = 0; i < read_length; i++)
            output[i] = audio_file->buffer[offset + i];

        // return read length
        return read_length;
    }
    else
    {
        // seek to the offset and read the items
        sf_seek(audio_file->file, offset / audio_file->info.channels, SEEK_SET);
        return sf_read_float(audio_file->file, output, length);
    }
}
