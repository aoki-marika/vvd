#pragma once

#include "chart.h"
#include "audio_track.h"
#include "track.h"
#include "scoring.h"

// the value used to signify that there is no current note/analog in a playbacks current_* array
#define PLAYBACK_CURRENT_NONE -1

typedef struct
{
    // the chart this playback is playing
    Chart *chart;

    // the audio track for chart
    AudioTrack *audio_track;

    // the track for this playback to control
    Track *track;

    // the scoring for this playback to use
    Scoring *scoring;

    // whether or not this playback has begun playing
    // set to true after the current time is at or passed start_time in playback_update
    bool started;

    // the time, in milliseconds, that this playback should begin playing at
    double start_time;

    // the index of last tempo that this playback reached in charts tempos
    int tempo_index;

    // the indexes of the currents notes and analogs from the last call to playback_update
    int current_bt_notes[CHART_BT_LANES];
    int current_fx_notes[CHART_FX_LANES];
    int current_analogs[CHART_ANALOG_LANES];

    // the index of the start point of the current segment of each analog in current_analogs
    // relative to current_analogs points
    int current_analogs_points[CHART_ANALOG_LANES];
} Playback;

Playback *playback_create(Chart *chart, AudioTrack *audio_track, Track *track, Scoring *scoring);
void playback_free(Playback *playback);

// start playing the given playback with a given delay
// delay is how many milliseconds after calling playback_start that playback will start
void playback_start(Playback *playback, double delay);

// update the playback state for the current time and draw the given playbacks track
// must call playback_start first
// returns whether or not playback is finished
bool playback_update(Playback *playback);

// tell the given playback that the bt/fx button for the given lane has changed states to the given state
void playback_bt_state_changed(Playback *playback, int lane, bool pressed);
void playback_fx_state_changed(Playback *playback, int lane, bool pressed);
