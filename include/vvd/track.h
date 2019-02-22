#pragma once

#include "chart.h"
#include "program.h"
#include "mesh.h"

//
// TRACK
//

// the size of the tracks note area
#define TRACK_NOTES_WIDTH 0.75f

// the size of an analog gutter on the track
#define TRACK_GUTTER_WIDTH 0.15f

// the full width of the track
#define TRACK_WIDTH TRACK_NOTES_WIDTH + (TRACK_GUTTER_WIDTH * CHART_ANALOG_LANES)

// the length of the track
#define TRACK_LENGTH 12.0f

// the number of beats that should be visible at 1x speed
#define TRACK_BEAT_SPEED 8

//
// CAMERA
//

// the visual offset, on the y axis, of the track
// this is used so the track is higher up on the screen
#define TRACK_VISUAL_OFFSET -3.75f

// the offset, on the z axis, of the camera from the start of the track
#define TRACK_CAMERA_OFFSET -1.0f

//
// BUFFER
//

// the number of beats in a single buffer chunk
#define TRACK_BUFFER_CHUNK_BEATS 4

// the number of extra buffer chunks that are loaded before and after the current buffer chunks
#define TRACK_EXTRA_BUFFER_CHUNKS 2

//
// NOTES AND BARS
//

// the height of a measure or beat bar on the track
#define TRACK_BAR_HEIGHT 0.025f

// the height of a bt or fx chip on the track
#define TRACK_CHIP_HEIGHT 0.075f

// the width of a bt note on the track
#define TRACK_BT_WIDTH TRACK_NOTES_WIDTH / CHART_BT_LANES

typedef struct
{
    // the offset of the vertices to draw in the mesh
    int offset;

    // the size of the vertices to draw in the mesh
    size_t size;
} TrackLaneVertices;

typedef struct
{
    // the chart this track is displaying
    Chart *chart;

    // the lane (track background) program and mesh
    Program *lane_program;
    GLuint uniform_lane_lane_id;
    Mesh *lane_mesh;

    // the measure bars program and mesh
    Program *measure_bars_program;
    Mesh *measure_bars_mesh;

    // the bt chips and holds programs and meshes
    Program *bt_chips_program, *bt_holds_program;
    Mesh *bt_chips_mesh, *bt_holds_mesh;
    TrackLaneVertices *bt_lanes_vertices[CHART_BT_LANES];

    // the fx chips and holds programs and meshes
    Program *fx_chips_program, *fx_holds_program;
    Mesh *fx_chips_mesh, *fx_holds_mesh;
    TrackLaneVertices *fx_lanes_vertices[CHART_FX_LANES];

    // the current time this track is scrolled to, in milliseconds
    double time;

    // the index of the current tempo this track is scrolling at
    int tempo_index;

    // the current speed this track is scrolling at
    double speed;

    // the current buffer chunk this track is scrolled to
    int buffer_position;
} Track;

Track *track_create(Chart *chart);
void track_free(Track *track);

// set the given tracks scroll speed
void track_set_speed(Track *track, double speed);

// draw the given tracks state at the given time in milliseconds
void track_draw(Track *track, double time);
