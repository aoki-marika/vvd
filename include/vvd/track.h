#pragma once

#include "chart.h"
#include "program.h"
#include "mesh.h"

// the size of the track
#define TRACK_WIDTH 1.0f
#define TRACK_LENGTH 10.0f

// the visual offset, on the y axis, of the track
// this is used so the track is higher up on the screen
#define TRACK_VISUAL_OFFSET -7.0f

// the offset, on the z axis, of the camera from the start of the track
#define TRACK_CAMERA_OFFSET -11.5f

typedef struct
{
    // the chart this track is displaying
    Chart *chart;

    // the time, in ms, of each of charts beats
    // this array is equal in length and represents values at the same index as chart->beats
    double *beat_times;

    // the time, in ms, of each of charts tempos
    // this array is equal in length and represents values at the same index as chart->tempos
    double *tempo_times;

    // the lane (track background) program and mesh
    Program *lane_program;
    Mesh *lane_mesh;
} Track;

Track *track_create(Chart *chart);
void track_free(Track *track);

// draw the given tracks state at the given time (ms from the beginning of the track) and hi-speed
void track_draw(Track *track, double time, double speed);
