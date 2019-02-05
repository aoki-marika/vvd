#pragma once

#include <stdbool.h>
#include <portaudio.h>

#include "audio_file.h"

// max values
#define AUDIO_MAX_FILES 32

// audio file types
#define AUDIO_SAMPLE 0
#define AUDIO_TRACK 1

typedef struct
{
    uint64_t position;
    bool finished;
    AudioFile *file;
} AudioPlaying;

typedef struct
{
    PaStream *stream;
    int current_id; //the last assigned audio file id, incremented for the next

    // samples are audio files that are loaded into and streamed from memory, e.g. sound effects
    int num_samples;
    AudioFile *samples[AUDIO_MAX_FILES];

    // tracks are audio files that are streamed from disk, e.g. a song
    int num_tracks;
    AudioFile *tracks[AUDIO_MAX_FILES];

    // the currently playing audio tracks, removed upon completion
    int num_playing;
    AudioPlaying playing[AUDIO_MAX_FILES];
} Audio;

Audio *audio_get();
void audio_free(Audio *audio);

// process the current state of the given audio
void audio_update(Audio *audio);

// add an audio file of the given type
// type is either AUDIO_SAMPLE or AUDIO_TRACK
// path is the path to the audio file to load on disk
// returns the id of the added file
int audio_add(Audio *audio, int type, const char *path);

// starts playing an audio file of the given type and id
void audio_play(Audio *audio, int type, int id);
