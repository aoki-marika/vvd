#pragma once

#include <bass/bass.h>

typedef struct
{
	HSTREAM stream;
} AudioTrack;

AudioTrack *audio_track_create(const char *path);
void audio_track_free(AudioTrack *track);

// restart and play the given audio track
void audio_track_play(AudioTrack *track);

// returns the current playback of the given track, in milliseconds
double audio_track_position(AudioTrack *track);
