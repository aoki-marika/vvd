#pragma once

#include "chart.h"
#include "program.h"
#include "mesh.h"

// the size, in vertices, of a measure bar in a measure bar mesh
#define MEASURE_BAR_MESH_MEASURE_BAR_SIZE MESH_VERTICES_QUAD

typedef struct
{
    // the chart for this mesh to get measures from
    Chart *chart;

    // the program for this mesh
    Program *program;
    GLuint uniform_speed_id, uniform_position_id;

    // the mesh for a bar in this mesh
    Mesh *mesh;

    // the y position of each measure bar, at 1x speed
    float *measure_bar_positions;

    // the subbeat of each measure bar
    int *measure_bar_subbeats;
} MeasureBarMesh;

MeasureBarMesh *measure_bar_mesh_create(Chart *chart);
void measure_bar_mesh_free(MeasureBarMesh *mesh);

// draw the given meshes measure bars that are in range of start_subbeat and end_subbeat as speed, with the given projection, view, and model matrices
void measure_bar_mesh_draw(MeasureBarMesh *mesh, mat4_t projection, mat4_t view, mat4_t model, uint16_t start_subbeat, uint16_t end_subbeat, double speed);
