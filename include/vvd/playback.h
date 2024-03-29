#pragma once

#include "chart.h"
#include "audio_track.h"
#include "track.h"
#include "scoring.h"

// the size in subbeats of a tick
#define PLAYBACK_TICK_SIZE 12

// if the current tempo is >= this value, then the tick size should double, halving the tick rate
#define PLAYBACK_HALF_TICK_RATE_BPM 256

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

    // the speed this playback is currently scrolling at
    double speed;

    // whether or not this playback has begun playing
    // set to true after the current time is at or passed start_time in playback_update
    bool started;

    // the time, in milliseconds, that this playback should begin playing at
    double start_time;

    // the last tick this playback was updated at
    int last_tick;

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

// set the given playbacks scroll speed
void playback_set_speed(Playback *playback, double speed);

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
