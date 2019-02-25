#pragma once

#include "chart.h"
#include "audio_track.h"
#include "track.h"

typedef struct
{
    // the chart this playback is playing
    Chart *chart;

    // the audio track for chart
    AudioTrack *audio_track;

    // the track for this playback to control
    Track *track;

    // whether or not this playback has begun playing
    // set to true after the current time is at or passed start_time in playback_update
    bool started;

    // the time, in milliseconds, that this playback should begin playing at
    double start_time;
} Playback;

Playback *playback_create(Chart *chart, AudioTrack *audio_track, Track *track);
void playback_free(Playback *playback);

// start playing the given playback with a given delay
// delay is how many milliseconds after calling playback_start that playback will start
void playback_start(Playback *playback, double delay);

// update the playback state for the current time and draw the given playbacks track
// must call playback_start first
// returns whether or not playback is finished
bool playback_update(Playback *playback);
