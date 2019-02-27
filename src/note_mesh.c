#include "note_mesh.h"

#include <string.h>
#include <linux/limits.h>

#include "track.h"

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

    // create the chip and hold meshes
    // todo: this does not need to allocate this much
    // there should never be a case where every note is shown and the chart maxed out all note lanes
    // some sort of calculation based off of the minimum speed and extra buffer chunks
    size_t mesh_size = CHART_NOTES_MAX * num_lanes * MESH_VERTICES_QUAD;
    mesh->chips_mesh = mesh_create(mesh_size, mesh->chips_program, GL_DYNAMIC_DRAW);
    mesh->holds_mesh = mesh_create(mesh_size, mesh->holds_program, GL_DYNAMIC_DRAW);

    // return the note mesh
    return mesh;
}

void note_mesh_free(NoteMesh *mesh)
{
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
                hold_vertices_index += MESH_VERTICES_QUAD;
            else
                chip_vertices_index += MESH_VERTICES_QUAD;
        }
    }

    // set the meshes chips and holds sizes
    mesh->chips_size = chip_vertices_index;
    mesh->holds_size = hold_vertices_index;
}

void note_mesh_draw_holds(NoteMesh *mesh, mat4_t projection, mat4_t view, mat4_t model)
{
    // additive blending for holds
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // draw the holds
    program_use(mesh->holds_program);
    program_set_matrices(mesh->holds_program, projection, view, model);
    mesh_draw(mesh->holds_mesh, 0, mesh->holds_size);
}

void note_mesh_draw_chips(NoteMesh *mesh, mat4_t projection, mat4_t view, mat4_t model)
{
    // normal blending for chips
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // draw the chips
    program_use(mesh->chips_program);
    program_set_matrices(mesh->chips_program, projection, view, model);
    mesh_draw(mesh->chips_mesh, 0, mesh->chips_size);
}
