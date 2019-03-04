#pragma once

#include "judgement.h"
#include "chart.h"
#include "program.h"
#include "mesh.h"
#include "measure_bar_mesh.h"
#include "note_mesh.h"
#include "analog_mesh.h"

//
// TRACK
//

// the size of the tracks note area
#define TRACK_NOTES_WIDTH 0.75f

// the size of an analog gutter on the track
#define TRACK_GUTTER_WIDTH 0.15f

// the full width of the track
#define TRACK_WIDTH (TRACK_NOTES_WIDTH + (TRACK_GUTTER_WIDTH * CHART_ANALOG_LANES))

// the length of the track
#define TRACK_LENGTH 12.0f

// the number of beats that should be visible at 1x speed
#define TRACK_BEAT_SPEED 8

//
// BEAM
//

// the time in milliseconds for a beam to fade out after triggering
#define TRACK_BEAM_DURATION 100

// the alpha of a bt beam when its triggered
#define TRACK_BT_BEAM_ALPHA 0.5

// the alpha of an fx beam when its triggered
#define TRACK_FX_BEAM_ALPHA 0.125

//
// CAMERA
//

// the visual offset, on the y axis, of the track
// this is used so the track is higher up on the screen
#define TRACK_VISUAL_OFFSET -3.75f

// the offset, on the z axis, of the camera from the start of the track
#define TRACK_CAMERA_OFFSET -1.0f

//
// NOTES, ANALOGS, AND BARS
//

// todo: move these to note/analog mesh headers?

// the height of a measure or beat bar on the track
// todo: proper bar height? not sure if this is resized on speed change
#define TRACK_BAR_HEIGHT 0.025f

// the height of bt and fx chips
// todo: move to note_mesh.h?
#define TRACK_CHIP_HEIGHT 0.075f

// the width of a bt note on the track
#define TRACK_BT_WIDTH TRACK_NOTES_WIDTH / CHART_BT_LANES

// the width of an fx note on the track
#define TRACK_FX_WIDTH TRACK_BT_WIDTH * (CHART_BT_LANES / CHART_FX_LANES)

// the width of a non-slam analog on the track
#define TRACK_ANALOG_WIDTH TRACK_GUTTER_WIDTH * 1.1

// the height in subbeats of an analog slam
// todo: slams seem to resize based on tempo
#define TRACK_ANALOG_SLAM_SUBBEATS 8

typedef struct
{
    // the current judgement of this beam
    Judgement judgement;

    // the last time that judgement was set on this beam
    double time;
} BeamState;

typedef struct
{
    // the chart this track is displaying
    Chart *chart;

    // the lane (track background) program and mesh
    Program *lane_program;
    GLuint uniform_lane_lane_id;
    Mesh *lane_mesh;

    // the beam program
    Program *beam_program;
    GLuint uniform_beam_judgement_id;
    GLuint uniform_beam_alpha_id;

    // the bt and fx beam meshes
    Mesh *bt_beam_mesh, *fx_beam_mesh;

    // the current state of each bt and fx lanes beam
    BeamState bt_beam_states[CHART_BT_LANES];
    BeamState fx_beam_states[CHART_FX_LANES];

    // the measure bar, bt, fx, and analog meshes of this track
    MeasureBarMesh *measure_bar_mesh;
    NoteMesh *bt_mesh, *fx_mesh;
    AnalogMesh *analog_mesh;
} Track;

Track *track_create(Chart *chart);
void track_free(Track *track);

// get the size of a beat on the track at 1x speed
// multiply by the current speed to get the proper draw size
float track_beat_size();

// get the position of a subbeat on the track at the 1x speed
// multiply by the current speed to get the proper draw position
// e.g. track_subbeat_position(100) * 4.5 is subbeat 100 at 4.5x speed
float track_subbeat_position(double subbeat);

// trigger the bt/fx beam on the given lane with the given judgement
void track_bt_beam(Track *track, int lane, Judgement judgement);
void track_fx_beam(Track *track, int lane, Judgement judgement);

// draw the given tracks state at the given tempo, subbeat, and speed
// tempo_index should be in the given tracks charts tempos
void track_draw(Track *track, int tempo_index, double subbeat, double speed);
