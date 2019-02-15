#pragma once

#include <bass/bass.h>

typedef struct
{
	HSAMPLE sample;
} AudioSample;

AudioSample *audio_sample_create(const char *path);
void audio_sample_free(AudioSample *sample);

// restart and play the given audio sample
void audio_sample_play(AudioSample *sample);
