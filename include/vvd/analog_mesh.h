#pragma once

#include "program.h"
#include "mesh.h"
#include "chart.h"

// the size, in vertices, of a segment/slam/slam tail in an analog mesh
#define ANALOG_MESH_SEGMENT_SIZE MESH_VERTICES_QUAD
#define ANALOG_MESH_SLAM_SIZE MESH_VERTICES_QUAD
#define ANALOG_MESH_SLAM_TAIL_SIZE MESH_VERTICES_QUAD

// the width of a non-slam analog segment
#define ANALOG_MESH_SEGMENT_WIDTH (TRACK_GUTTER_WIDTH * 1.1f)

// the z offset of analogs in an analog mesh
#define ANALOG_MESH_RAISE -0.005f

// the speed that is added to the given speed for the final speed of an analog meshes scrolling
#define ANALOG_MESH_SPEED_OFFSET 0.05f

// the height in subbeats of an analog slam at 1x height scale
#define ANALOG_MESH_SLAM_SUBBEATS 8

// the minimum height scale for slams in an analog mesh
#define ANALOG_MESH_SLAM_MIN_HEIGHT_SCALE 0.5

typedef struct
{
    // the chart for this mesh to get analogs from
    Chart *chart;

    // the program for this mesh
    Program *program;
    GLuint uniform_speed_id, uniform_lane_id;

    // the mesh for analogs in this mesh
    Mesh *mesh;
} AnalogMesh;

AnalogMesh *analog_mesh_create(Chart *chart);
void analog_mesh_free(AnalogMesh *mesh);

// draw the given meshes segments that are in range of start_subbeat and end_subbeat at the given speed, with the given projection, view, and model matrices
// analog meshes scroll themselves from 0,0,0 based on the given speed and position
// model should be used for offsetting the scrolling from 0,0,0
// position should be the current scroll position of this mesh at 1x speed
void analog_mesh_draw(AnalogMesh *mesh,
                      mat4_t projection,
                      mat4_t view,
                      mat4_t model,
                      double position,
                      uint16_t start_subbeat,
                      uint16_t end_subbeat,
                      double speed);
