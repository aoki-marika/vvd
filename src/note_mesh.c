#include "note_mesh.h"

#include <string.h>
#include <assert.h>
#include <linux/limits.h>

#include "track.h"
#include "shared.h"

void load_notes(NoteMesh *mesh)
{
    // the index of the current hold vertices
    int hold_vertices_index = 0;

    for (int l = 0; l < mesh->num_lanes; l++)
    {
        // the x position of notes in this lane
        float lane_position = (l * mesh->note_width) - (TRACK_NOTES_WIDTH / 2);

        // create the chip vertices for this lane
        mesh_set_vertices_quad(mesh->chips_mesh,
                               l * NOTE_MESH_CHIP_SIZE,
                               mesh->note_width,
                               NOTE_MESH_CHIP_HEIGHT,
                               vec3(lane_position, 0, 0));

        for (int n = 0; n < mesh->num_notes[l]; n++)
        {
            Note *note = &mesh->notes[l][n];

            // get the position of the current note
            vec3_t position = vec3(lane_position,
                                   track_subbeat_position(note->start_subbeat),
                                   0);

            // set position and continue if the current note is a chip, as thats all chips need
            if (!note->hold)
            {
                mesh->chip_positions[l][n] = position.y;
                continue;
            }

            // get the size of the current hold
            vec3_t size = vec3(mesh->note_width,
                               track_subbeat_position(note->end_subbeat) - position.y,
                               0);

            // add the hold vertices to the holds mesh
            mesh_set_vertices_quad(mesh->holds_mesh,
                                   hold_vertices_index,
                                   size.x, size.y,
                                   position);

            // increment the hold vertices index
            hold_vertices_index += NOTE_MESH_HOLD_SIZE;
        }
    }
}

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
    mesh->chip_positions = malloc(num_lanes * sizeof(float *));
    mesh->chips_removed = calloc(num_lanes, sizeof(bool *));
    mesh->current_hold_indexes = malloc(num_lanes * sizeof(int));
    mesh->current_hold_states = malloc(num_lanes * sizeof(HoldState));

    for (int i = 0; i < num_lanes; i++)
    {
        mesh->chip_positions[i] = malloc(num_notes[i] * sizeof(float));
        mesh->chips_removed[i] = calloc(num_notes[i], sizeof(bool));
        mesh->current_hold_indexes[i] = INDEX_NONE;
        mesh->current_hold_states[i] = HoldStateDefault;
    }

    // create the chip program
    char chip_fragment_path[PATH_MAX];
    strcpy(chip_fragment_path, type_name);
    strcat(chip_fragment_path, "_chip.fs");

    mesh->chips_program = program_create("chip.vs", chip_fragment_path, true);
    mesh->uniform_chips_speed_id = program_get_uniform_id(mesh->chips_program, "speed");
    mesh->uniform_chips_position_id = program_get_uniform_id(mesh->chips_program, "position");

    // create the hold program
    char hold_fragment_path[PATH_MAX];
    strcpy(hold_fragment_path, type_name);
    strcat(hold_fragment_path, "_hold.fs");

    mesh->holds_program = program_create("hold.vs", hold_fragment_path, true);
    mesh->uniform_holds_speed_id = program_get_uniform_id(mesh->holds_program, "speed");
    mesh->uniform_holds_state_id = program_get_uniform_id(mesh->holds_program, "state");

    // create the chip and hold meshes
    mesh->chips_mesh = mesh_create(num_lanes * NOTE_MESH_CHIP_SIZE, mesh->chips_program, GL_STATIC_DRAW);

    int num_holds = 0;
    for (int l = 0; l < num_lanes; l++)
        for (int n = 0; n < num_notes[l]; n++)
            if (notes[l][n].hold)
                num_holds++;

    mesh->holds_mesh = mesh_create(num_holds * NOTE_MESH_HOLD_SIZE, mesh->holds_program, GL_STATIC_DRAW);

    // load the notes
    load_notes(mesh);

    // return the note mesh
    return mesh;
}

void note_mesh_free(NoteMesh *mesh)
{
    // free all the allocated properties
    free(mesh->current_hold_indexes);
    free(mesh->current_hold_states);

    for (int i = 0; i < mesh->num_lanes; i++)
    {
        free(mesh->chip_positions[i]);
        free(mesh->chips_removed[i]);
    }

    free(mesh->chip_positions);
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
    if (index != INDEX_NONE)
    {
        // assert that the index is valid
        assert(index == INDEX_NONE || (index >= 0 && index < mesh->num_notes[lane]));

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

void note_mesh_draw_chips(NoteMesh *mesh,
                          mat4_t projection,
                          mat4_t view,
                          mat4_t model,
                          uint16_t start_subbeat,
                          uint16_t end_subbeat,
                          double speed)
{
    // normal blending for chips
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // draw the chips
    program_use(mesh->chips_program);
    program_set_matrices(mesh->chips_program, projection, view, model);
    glUniform1f(mesh->uniform_chips_speed_id, speed);
    mesh_draw_start(mesh->chips_mesh);

    for (int l = 0; l < mesh->num_lanes; l++)
    {
        for (int n = 0; n < mesh->num_notes[l]; n++)
        {
            Note *note = &mesh->notes[l][n];

            // skip holds
            if (note->hold)
                continue;

            // draw the current chip if its in range and not removed
            if (note->start_subbeat >= start_subbeat && //after start_subbeat
                note->start_subbeat <= end_subbeat && //before end_subbeat
                !mesh->chips_removed[l][n]) //not removed
            {
                glUniform1f(mesh->uniform_chips_position_id, mesh->chip_positions[l][n]);
                mesh_draw_vertices(mesh->chips_mesh, l * NOTE_MESH_CHIP_SIZE, NOTE_MESH_CHIP_SIZE);
            }
        }
    }

    mesh_draw_end(mesh->chips_mesh);
}

void note_mesh_draw_holds(NoteMesh *mesh,
                          mat4_t projection,
                          mat4_t view,
                          mat4_t model,
                          uint16_t start_subbeat,
                          uint16_t end_subbeat,
                          double speed)
{
    // additive blending for holds
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // draw the holds
    program_use(mesh->holds_program);
    program_set_matrices(mesh->holds_program, projection, view, model);
    glUniform1f(mesh->uniform_holds_speed_id, speed);
    mesh_draw_start(mesh->holds_mesh);

    int vertices_index = 0;
    for (int l = 0; l < mesh->num_lanes; l++)
    {
        for (int n = 0; n < mesh->num_notes[l]; n++)
        {
            Note *note = &mesh->notes[l][n];

            // skip chips
            if (!note->hold)
                continue;

            // apply the current hold state if the current hold is the current hold for the current lane
            if (mesh->current_hold_indexes[l] == n)
            {
                HoldState state = mesh->current_hold_states[l];
                glUniform1i(mesh->uniform_holds_state_id, state);
            }

            // draw the hold if its in range
            if (note->end_subbeat >= start_subbeat && //after start_subbeat
                note->start_subbeat <= end_subbeat) //before end_subbeat
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
