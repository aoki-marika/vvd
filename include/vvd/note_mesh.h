#pragma once

#include <stdint.h>

#include "program.h"
#include "mesh.h"
#include "chart.h"

typedef struct
{
    // the programs and meshes for chips and holds of this mesh
    Program *chips_program, *holds_program;
    Mesh *chips_mesh, *holds_mesh;

    // the current size of the vertices in the chips and holds meshes, respectively
    int chips_size, holds_size;
} NoteMesh;

NoteMesh *note_mesh_create(const char *type_name, int num_lanes);
void note_mesh_free(NoteMesh *mesh);

// load the note vertices for the given notes that are in range of start_subbeat to end_subbeat into the given mesh
void note_mesh_load(NoteMesh *mesh,
                    int num_lanes,
                    int num_notes[num_lanes],
                    Note *notes[num_lanes],
                    float note_width,
                    uint16_t start_subbeat,
                    uint16_t end_subbeat,
                    double speed);

// draw the given meshes holds with the given projection, view, and model matrices
void note_mesh_draw_holds(NoteMesh *mesh, mat4_t projection, mat4_t view, mat4_t model);

// draw the given meshes chips with the given projection, view, and model matrices
void note_mesh_draw_chips(NoteMesh *mesh, mat4_t projection, mat4_t view, mat4_t model);
