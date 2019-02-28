#pragma once

#include <stdint.h>

#include "program.h"
#include "mesh.h"
#include "chart.h"

// the size, in vertices, of a chip or hold in a note mesh
#define NOTE_MESH_CHIP_SIZE MESH_VERTICES_QUAD
#define NOTE_MESH_HOLD_SIZE MESH_VERTICES_QUAD

typedef struct
{
    // the number of note lanes in this mesh
    int num_lanes;

    // the number of notes of each lane for this mesh
    int *num_notes;

    // the notes of each lane for this mesh
    Note **notes;

    // the width of a note in this mesh
    float note_width;

    // the programs and meshes for chips and holds of this mesh
    Program *chips_program, *holds_program;
    Mesh *chips_mesh, *holds_mesh;

    // the current size of the vertices in the chips and holds meshes, respectively
    int chips_size, holds_size;

    // the index of and size of the currently loaded notes for each lane in notes
    int *loaded_notes_index;
    int *loaded_notes_size;
} NoteMesh;

// create a note mesh for the given type and notes
NoteMesh *note_mesh_create(const char *type_name,
                           int num_lanes,
                           int num_notes[num_lanes],
                           Note *notes[num_lanes],
                           float note_width);

void note_mesh_free(NoteMesh *mesh);

// load the note vertices of the given mesh that are between start_subbeat and end_subbeat into the given mesh
void note_mesh_load(NoteMesh *mesh, uint16_t start_subbeat, uint16_t end_subbeat, double speed);

// draw the given meshes holds with the given projection, view, and model matrices
void note_mesh_draw_holds(NoteMesh *mesh, mat4_t projection, mat4_t view, mat4_t model);

// draw the given meshes chips with the given projection, view, and model matrices
void note_mesh_draw_chips(NoteMesh *mesh, mat4_t projection, mat4_t view, mat4_t model);
