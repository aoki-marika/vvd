#pragma once

#include "chart.h"
#include "program.h"
#include "mesh.h"

// max values
#define TRACK_MEASURE_BARS_MAX 512

// the size of the track
#define TRACK_WIDTH 1.0f
#define TRACK_LENGTH 10.0f

// the visual offset, on the y axis, of the track
// this is used so the track is higher up on the screen
#define TRACK_VISUAL_OFFSET -7.0f

// the offset, on the z axis, of the camera from the start of the track
#define TRACK_CAMERA_OFFSET -11.5f

// the number of beats that should be visible at 1x speed
#define TRACK_BEAT_SPEED 8

// the height of a measure or beat bar on the track
#define TRACK_BAR_HEIGHT 0.025f

// the height of a bt or fx chip on the track
#define TRACK_CHIP_HEIGHT 0.075f

// the width of a bt note on the track
#define TRACK_BT_WIDTH TRACK_WIDTH / CHART_BT_LANES

typedef struct
{
    // the chart this track is displaying
    Chart *chart;

    // the lane (track background) program and mesh
    Program *lane_program;
    Mesh *lane_mesh;

    // the measure bars program and mesh
    Program *measure_bars_program;
    Mesh *measure_bars_mesh;

    // the bt notes program and mesh
    Program *bt_notes_program;
    Mesh *bt_notes_mesh;

    // the fx notes program and mesh
    Program *fx_notes_program;
    Mesh *fx_notes_mesh;

    // the times, in ms, of each tempo in chart->tempos
    double *tempo_times;

    // the subbeat of each tempo in chart->tempos
    // todo: removeme...?
    int *tempo_subbeats;

    // the subbeat this tracks chart ends on
    // todo: removeme...?
    int end_subbeat;

    // the index of the current tempo this track is at
    int tempo_index;

    // the current speed this track is scrolling at
    double speed;
} Track;

Track *track_create(Chart *chart);
void track_free(Track *track);

// draw the given tracks state at the given time (ms from the beginning of the track) and hi-speed
void track_draw(Track *track, double time, double speed);
