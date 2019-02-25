#pragma once

#include "program.h"
#include "mesh.h"
#include "chart.h"

typedef struct
{
    // the program for this mesh
    Program *program;
    GLuint program_lane_id;

    // the meshes for each lane of this analog mesh
    Mesh *lane_meshes[CHART_ANALOG_LANES];

    // the current size of the vertices in the each lanes vertices
    int lane_sizes[CHART_ANALOG_LANES];
} AnalogMesh;

AnalogMesh *analog_mesh_create();
void analog_mesh_free(AnalogMesh *mesh);

// load analog segment vertices from the given chart that are between start_subbeat and end_subbeat into the given mesh
void analog_mesh_load(AnalogMesh *mesh, Chart *chart, uint16_t start_subbeat, uint16_t end_subbeat, double speed);

// draw the given meshes segments with the given projection, view, and model matrices
void analog_mesh_draw(AnalogMesh *mesh, mat4_t projection, mat4_t view, mat4_t model);
