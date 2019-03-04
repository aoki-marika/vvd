#pragma once

#include "program.h"
#include "mesh.h"
#include "chart.h"

// the size, in vertices, of a segment/slam/slam tail in an analog mesh
#define ANALOG_MESH_SEGMENT_SIZE MESH_VERTICES_QUAD
#define ANALOG_MESH_SLAM_SIZE MESH_VERTICES_QUAD
#define ANALOG_MESH_SLAM_TAIL_SIZE MESH_VERTICES_QUAD

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

// draw the given meshes segments that are in range of start_subbeat and end_subbeat as speed, with the given projection, view, and model matrices
void analog_mesh_draw(AnalogMesh *mesh, mat4_t projection, mat4_t view, mat4_t model, uint16_t start_subbeat, uint16_t end_subbeat, double speed);
