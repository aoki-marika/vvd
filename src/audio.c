#include "audio.h"

#include <stdlib.h>
#include <bass/bass.h>

#include "bass_utils.h"

Audio *audio_create()
{
    Audio *audio = malloc(sizeof(Audio));

    // set the properties
    audio->device = -1;

    // ensure the loaded bass version is correct
    if (HIWORD(BASS_GetVersion()) != BASSVERSION)
        bass_error("an incorrect version of bass was loaded");

    // init bass
    if (!BASS_Init(audio->device, 44100, 0, 0, NULL))
        bass_error("unable to initialize output device");

    return audio;
}

void audio_free(Audio *audio)
{
    BASS_Free();
    free(audio);
}
