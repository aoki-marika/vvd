#pragma once

#include <stdint.h>
#include <stdbool.h>

// max values
#define CHART_STR_MAX 1024
#define CHART_EVENTS_MAX 256
#define CHART_NOTES_MAX 4096

// number of lanes per note type
#define CHART_BT_LANES 4
#define CHART_FX_LANES 2

// index of lane names per note type
#define CHART_BT_LANE_A 0
#define CHART_BT_LANE_B 1
#define CHART_BT_LANE_C 2
#define CHART_BT_LANE_D 3

#define CHART_FX_LANE_L 0
#define CHART_FX_LANE_R 1

// max number of subbeats per beat
// todo: not sure this is the proper term for this
#define CHART_BEAT_MAX_SUBBEATS 48

typedef struct
{
    // the top and bottom values of the time signature for this beat
    uint8_t numerator;
    uint8_t denominator;

    // the measure this beat starts at
    uint16_t measure;

    // the subbeat this beat starts at
    // note: not relative to measure
    uint16_t subbeat;
} Beat;

typedef struct
{
    // the beats per minute of this tempo
    float bpm;

    // the time in milliseconds that this tempo starts at
    double time;

    // the subbeat this tempo starts at
    uint16_t subbeat;
} Tempo;

typedef struct
{
    // the subbeat this note starts at
    uint16_t start_subbeat;

    // whether or not this note is a hold note
    bool hold;

    // the subbeat this note ends at
    // only applicable to hold notes
    uint16_t end_subbeat;
} Note;

typedef struct
{
    // metadata about this chart
    char *title;
    char *artist;
    char *effector;
    char *illustrator;

    // the difficulty rating of this chart (1-20)
    uint8_t rating;

    // the offset (in ms) for the audio of this chart
    double offset;

    // the different beats of this chart
    int num_beats;
    Beat *beats;

    // the different tempos of this chart
    int num_tempos;
    Tempo *tempos;

    // the bt notes of this chart
    int num_bt_notes[CHART_BT_LANES];
    Note *bt_notes[CHART_BT_LANES];

    // the fx notes of this chart
    int num_fx_notes[CHART_FX_LANES];
    Note *fx_notes[CHART_FX_LANES];

    // the subbeat this chart ends at
    uint16_t end_subbeat;

    // the total number of measures in this chart
    uint16_t num_measures;
} Chart;

Chart *chart_create(const char *path);
void chart_free(Chart *chart);

// add a new tempo to the given chart with the given values
void chart_add_tempo(Chart *chart, double bpm, uint16_t subbeat);
