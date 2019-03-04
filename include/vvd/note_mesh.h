#pragma once

#include <stdint.h>

#include "program.h"
#include "mesh.h"
#include "chart.h"

// the size, in vertices, of a chip/hold in a note mesh
#define NOTE_MESH_CHIP_SIZE MESH_VERTICES_QUAD
#define NOTE_MESH_HOLD_SIZE MESH_VERTICES_QUAD

// the draw height of a chip note
#define NOTE_MESH_CHIP_HEIGHT 0.075f

typedef enum
{
    // hold is not current
    HoldStateDefault,

    // hold is current and the user is not holding the button
    HoldStateError,

    // hold is current and the user is holding the button
    HoldStateCritical,
} HoldState;

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
    GLuint uniform_chips_speed_id, uniform_chips_position_id;
    GLuint uniform_holds_speed_id, uniform_holds_state_id;
    Mesh *chips_mesh, *holds_mesh;

    // the draw positions at 1x speed of each chip in notes
    float **chip_positions;

    // whether or not each chip from notes is removed
    bool **chips_removed;

    // the indexes of the current hold of each lane of this mesh, if any
    int *current_hold_indexes;

    // the state of each current hold of each lane of this mesh
    HoldState *current_hold_states;
} NoteMesh;

// create a note mesh for the given type and notes
NoteMesh *note_mesh_create(const char *type_name,
                           int num_lanes,
                           int num_notes[num_lanes],
                           Note *notes[num_lanes],
                           float note_width);

void note_mesh_free(NoteMesh *mesh);

// remove the vertices for the chip at the given lane and index from the given mesh
void note_mesh_remove_chip(NoteMesh *mesh, int lane, int index);

// set the given note meshes hold that takes on the current hold state
void note_mesh_set_current_hold(NoteMesh *mesh, int lane, int index);

// set the state of the current hold on the given lane to the given state
// this state is stored and applies between changes of the current hold
void note_mesh_set_current_hold_state(NoteMesh *mesh, int lane, HoldState state);

// draw the given meshes chips that are in range of start_subbeat and end_subbeat as speed, with the given projection, view, and model matrices
void note_mesh_draw_chips(NoteMesh *mesh, mat4_t projection, mat4_t view, mat4_t model, uint16_t start_subbeat, uint16_t end_subbeat, double speed);

// draw the given meshes holds that are in range of start_subbeat and end_subbeat as speed, with the given projection, view, and model matrices
void note_mesh_draw_holds(NoteMesh *mesh, mat4_t projection, mat4_t view, mat4_t model, uint16_t start_subbeat, uint16_t end_subbeat, double speed);
