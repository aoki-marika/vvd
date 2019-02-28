#include "note_mesh.h"

#include <string.h>
#include <assert.h>
#include <linux/limits.h>

#include "track.h"
#include "playback.h"

NoteMesh *note_mesh_create(const char *type_name,
                           int num_lanes,
                           int num_notes[num_lanes],
                           Note *notes[num_lanes],
                           float note_width)
{
    // create the note mesh
    NoteMesh *mesh = malloc(sizeof(NoteMesh));

    // set the meshes properties
    mesh->num_lanes = num_lanes;
    mesh->num_notes = num_notes;
    mesh->notes = notes;
    mesh->note_width = note_width;
    mesh->loaded_notes_index = malloc(num_lanes * sizeof(int));
    mesh->loaded_notes_size = malloc(num_lanes * sizeof(int));
    mesh->chips_removed = calloc(num_lanes, sizeof(bool *));
    mesh->current_hold_indexes = malloc(num_lanes * sizeof(int));
    mesh->current_hold_states = malloc(num_lanes * sizeof(HoldState));

    for (int i = 0; i < num_lanes; i++)
    {
        mesh->chips_removed[i] = calloc(num_notes[i], sizeof(bool));
        mesh->current_hold_indexes[i] = PLAYBACK_CURRENT_NONE;
        mesh->current_hold_states[i] = HoldStateDefault;
    }

    // create the chip program
    char chip_vertex_path[PATH_MAX];
    strcpy(chip_vertex_path, type_name);
    strcat(chip_vertex_path, "_chip.vs");

    char chip_fragment_path[PATH_MAX];
    strcpy(chip_fragment_path, type_name);
    strcat(chip_fragment_path, "_chip.fs");

    mesh->chips_program = program_create(chip_vertex_path, chip_fragment_path, true);

    // create the hold program
    char hold_vertex_path[PATH_MAX];
    strcpy(hold_vertex_path, type_name);
    strcat(hold_vertex_path, "_hold.vs");

    char hold_fragment_path[PATH_MAX];
    strcpy(hold_fragment_path, type_name);
    strcat(hold_fragment_path, "_hold.fs");

    mesh->holds_program = program_create(hold_vertex_path, hold_fragment_path, true);
    mesh->uniform_holds_state_id = program_get_uniform_id(mesh->holds_program, "state");

    // create the chip and hold meshes
    // todo: this does not need to allocate this much
    // there should never be a case where every note is shown and the chart maxed out all note lanes
    // maybe use some sort of calculation based off of the minimum speed and extra buffer chunks
    mesh->chips_mesh = mesh_create(CHART_NOTES_MAX * num_lanes * NOTE_MESH_CHIP_SIZE, mesh->chips_program, GL_DYNAMIC_DRAW);
    mesh->holds_mesh = mesh_create(CHART_NOTES_MAX * num_lanes * NOTE_MESH_HOLD_SIZE, mesh->holds_program, GL_DYNAMIC_DRAW);

    // return the note mesh
    return mesh;
}

void note_mesh_free(NoteMesh *mesh)
{
    // free all the allocated properties
    free(mesh->loaded_notes_index);
    free(mesh->loaded_notes_size);
    free(mesh->current_hold_indexes);
    free(mesh->current_hold_states);

    for (int i = 0; i < mesh->num_lanes; i++)
        free(mesh->chips_removed[i]);
    free(mesh->chips_removed);

    // free the meshes
    mesh_free(mesh->chips_mesh);
    mesh_free(mesh->holds_mesh);

    // free the programs
    program_free(mesh->chips_program);
    program_free(mesh->holds_program);

    // free the mesh
    free(mesh);
}

void note_mesh_load(NoteMesh *mesh,
                    uint16_t start_subbeat,
                    uint16_t end_subbeat,
                    double speed)
{
    // the index of the current chip and hold vertices
    int chip_vertices_index = 0;
    int hold_vertices_index = 0;

    for (int l = 0; l < mesh->num_lanes; l++)
    {
        // reset the current lanes loaded notes index and size
        mesh->loaded_notes_index[l] = 0;
        mesh->loaded_notes_size[l] = 0;

        // whether or not the current lanes loaded notes index was set
        bool index_set = false;

        for (int n = 0; n < mesh->num_notes[l]; n++)
        {
            Note *note = &mesh->notes[l][n];

            // skip notes that are before start_subbeat
            if (((note->hold) ? note->end_subbeat : note->start_subbeat) < start_subbeat)
                continue;

            // break this lane if this note is after end_subbeat
            // no notes after it can be before end_subbeat
            if (note->start_subbeat > end_subbeat)
                break;

            // set the loaded notes index if it has not been set yet
            if (!index_set)
            {
                mesh->loaded_notes_index[l] = n;
                index_set = true;
            }

            // update the loaded notes size for the current index
            mesh->loaded_notes_size[l] = (n + 1) - mesh->loaded_notes_index[l];

            // calculate the position of this note
            vec3_t position = vec3((l * mesh->note_width) - (TRACK_NOTES_WIDTH / 2),
                                   track_subbeat_position(note->start_subbeat, speed),
                                   0);

            // calculate the size of the note
            vec3_t size = vec3(mesh->note_width,
                               TRACK_CHIP_HEIGHT,
                               0);

            // if this is a hold note then resize it from the start to end positions
            if (note->hold)
                size.y = track_subbeat_position(note->end_subbeat, speed) - position.y;

            // add the note vertices to the respective mesh for its type
            mesh_set_vertices_quad((note->hold) ? mesh->holds_mesh : mesh->chips_mesh,
                                   (note->hold) ? hold_vertices_index : chip_vertices_index,
                                   size.x, size.y,
                                   position);

            // increment the appropriate vertices_index for the next note
            if (note->hold)
                hold_vertices_index += NOTE_MESH_HOLD_SIZE;
            else
                chip_vertices_index += NOTE_MESH_CHIP_SIZE;
        }
    }

    // set the meshes chips and holds sizes
    mesh->chips_size = chip_vertices_index;
    mesh->holds_size = hold_vertices_index;
}

void note_mesh_remove_chip(NoteMesh *mesh, int lane, int index)
{
    // assert that the lane is valid
    assert(lane >= 0 && lane < mesh->num_lanes);

    // assert that the index is valid
    assert(index >= 0 && index < mesh->num_notes[lane]);

    // assert that the note is a chip
    assert(!mesh->notes[lane][index].hold);

    // mark the chip as removed
    mesh->chips_removed[lane][index] = true;
}

void note_mesh_set_current_hold(NoteMesh *mesh, int lane, int index)
{
    // assert that the lane is valid
    assert(lane >= 0 && lane < mesh->num_lanes);

    // only assert that the index and note are valid if the index is not none
    if (index != PLAYBACK_CURRENT_NONE)
    {
        // assert that the index is valid
        assert(index == PLAYBACK_CURRENT_NONE || (index >= 0 && index < mesh->num_notes[lane]));

        // assert that the note is a hold
        assert(mesh->notes[lane][index].hold);
    }

    // set the given meshes current hold for the given lane
    mesh->current_hold_indexes[lane] = index;
}

void note_mesh_set_current_hold_state(NoteMesh *mesh, int lane, HoldState state)
{
    // assert that the lane is valid
    assert(lane >= 0 && lane < mesh->num_lanes);

    // set the given lanes current hold state
    mesh->current_hold_states[lane] = state;
}

void note_mesh_draw_holds(NoteMesh *mesh, mat4_t projection, mat4_t view, mat4_t model)
{
    // additive blending for holds
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // draw the holds
    program_use(mesh->holds_program);
    program_set_matrices(mesh->holds_program, projection, view, model);
    mesh_draw_start(mesh->holds_mesh);

    int vertices_index = 0;
    for (int l = 0; l < mesh->num_lanes; l++)
    {
        for (int n = mesh->loaded_notes_index[l]; n < mesh->loaded_notes_index[l] + mesh->loaded_notes_size[l]; n++)
        {
            // skip chips
            if (!mesh->notes[l][n].hold)
                continue;

            // apply the current hold state if the current hold is the current hold for the current lane
            if (mesh->current_hold_indexes[l] == n)
            {
                HoldState state = mesh->current_hold_states[l];
                glUniform1i(mesh->uniform_holds_state_id, state);
            }

            // draw the hold
            mesh_draw_vertices(mesh->holds_mesh, vertices_index, NOTE_MESH_HOLD_SIZE);

            // reset the hold programs state to default if it was changed
            if (mesh->current_hold_indexes[l] == n)
                glUniform1i(mesh->uniform_holds_state_id, HoldStateDefault);

            // increment the vertices index
            vertices_index += NOTE_MESH_HOLD_SIZE;
        }
    }

    mesh_draw_end(mesh->holds_mesh);
}

void note_mesh_draw_chips(NoteMesh *mesh, mat4_t projection, mat4_t view, mat4_t model)
{
    // normal blending for chips
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // draw the chips
    program_use(mesh->chips_program);
    program_set_matrices(mesh->chips_program, projection, view, model);
    mesh_draw_start(mesh->chips_mesh);

    int vertices_index = 0;
    for (int l = 0; l < mesh->num_lanes; l++)
    {
        for (int n = mesh->loaded_notes_index[l]; n < mesh->loaded_notes_index[l] + mesh->loaded_notes_size[l]; n++)
        {
            // skip holds
            if (mesh->notes[l][n].hold)
                continue;

            // draw the current chip if it wasnt removed
            if (!mesh->chips_removed[l][n])
                mesh_draw_vertices(mesh->chips_mesh, vertices_index, NOTE_MESH_CHIP_SIZE);

            // increment the vertices index
            vertices_index += NOTE_MESH_CHIP_SIZE;
        }
    }

    mesh_draw_end(mesh->chips_mesh);
}
