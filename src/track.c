#include "track.h"

#include <stdlib.h>
#include <math.h>
#include <inttypes.h>

#define MATH_3D_IMPLEMENTATION
#include <arkanis/math_3d.h>

#include "screen.h"
#include "note_utils.h"

float subbeat_position(double subbeat, double speed)
{
    return subbeat / CHART_BEAT_SUBBEATS * speed / TRACK_BEAT_SPEED * (TRACK_LENGTH * 2);
}

void load_notes_mesh(Mesh *mesh,
                    Chart *chart,
                    int num_lanes,
                    int num_notes[num_lanes],
                    Note *notes[num_lanes],
                    vec3_t track_size,
                    float width,
                    double speed)
{
    for (int l = 0; l < num_lanes; l++)
    {
        for (int n = 0; n < num_notes[l]; n++)
        {
            Note *note = &notes[l][n];

            // calculate the start position of the note
            int lane = abs(l - (num_lanes - 1)); //lanes are rendered mirrored for some reason
            vec3_t position = vec3((lane * width) - (track_size.x / 2),
                                   subbeat_position(note->start_subbeat, speed),
                                   0);

            // calculate the size of the note
            vec3_t size = vec3(width,
                               TRACK_CHIP_HEIGHT,
                               0);

            // resize hold notes to their proper size
            if (note->hold)
                size.y = subbeat_position(note->end_subbeat, speed) - position.y;

            // add the note to the mesh
            mesh_set_vertices_quad_pos(mesh,
                                       ((l * CHART_NOTES_MAX) + n) * MESH_VERTICES_QUAD,
                                       size.x, size.y,
                                       position);
        }
    }
}

void reload_meshes(Track *track)
{
    // calculate reused values
    vec3_t track_size = vec3(TRACK_WIDTH * 2, TRACK_LENGTH * 2, 0);

    // draw all the measure bars
    int beat_index = 0;
    vec3_t measure_position = vec3(-track_size.x / 2, 0, 0);

    for (uint16_t i = 0; i < track->chart->num_measures; i++)
    {
        // create the measure bar for the current measure
        mesh_set_vertices_quad_pos(track->measure_bars_mesh,
                                   i * MESH_VERTICES_QUAD,
                                   track_size.x,
                                   TRACK_BAR_HEIGHT,
                                   measure_position);

        // increment the current beat if the next one is at or before the current measure
        if (beat_index + 1 < track->chart->num_beats &&
            i >= track->chart->beats[beat_index + 1].measure)
            beat_index++;

        // get the start and end position of this measure, as if it was in 4/4 time
        float start_position = subbeat_position((i - 1) * 4 * CHART_BEAT_SUBBEATS, track->speed);
        float end_position = subbeat_position(i * 4 * CHART_BEAT_SUBBEATS, track->speed);

        // get the size of the current measure as if it was 4/4 time by getting the difference between the start and end
        // then multiply by numerator/denominator to get the final size of the measure
        Beat *beat = &track->chart->beats[beat_index];
        float measure_size = (end_position - start_position) * ((float)beat->numerator / (float)beat->denominator);

        // increment measure_position.y for the next measure
        measure_position.y += measure_size;
    }

    // load the bt notes
    load_notes_mesh(track->bt_notes_mesh,
                   track->chart,
                   CHART_BT_LANES,
                   track->chart->num_bt_notes,
                   track->chart->bt_notes,
                   track_size,
                   TRACK_BT_WIDTH * 2,
                   track->speed);

    // load the fx notes
    load_notes_mesh(track->fx_notes_mesh,
                    track->chart,
                    CHART_FX_LANES,
                    track->chart->num_fx_notes,
                    track->chart->fx_notes,
                    track_size,
                    TRACK_BT_WIDTH * 4,
                    track->speed);
}

Track *track_create(Chart *chart)
{
    // create the track
    Track *track = malloc(sizeof(Track));

    // todo: can malloc the actual size of the arrays instead of max
    // the chart is already loaded and is not modified at any point during playback

    // set the track properties
    track->chart = chart;
    track->tempo_index = 0;
    track->speed = 0;

    // create the lane program and mesh
    track->lane_program = program_create("lane.vs", "lane.fs", true);
    track->lane_mesh = mesh_create(MESH_VERTICES_QUAD, track->lane_program);
    mesh_set_vertices_quad(track->lane_mesh, 0, TRACK_WIDTH, TRACK_LENGTH);

    // create the measure bars program and mesh
    track->measure_bars_program = program_create("measure_bar.vs", "measure_bar.fs", true);
    track->measure_bars_mesh = mesh_create(MESH_VERTICES_QUAD * TRACK_MEASURE_BARS_MAX, track->measure_bars_program);

    // create the bt notes program and mesh
    track->bt_notes_program = program_create("bt_note.vs", "bt_note.fs", true);
    track->bt_notes_mesh = mesh_create(MESH_VERTICES_QUAD * CHART_BT_LANES * CHART_NOTES_MAX, track->bt_notes_program);

    // create the fx notes program and mesh
    track->fx_notes_program = program_create("fx_note.vs", "fx_note.fs", true);
    track->fx_notes_mesh = mesh_create(MESH_VERTICES_QUAD * CHART_FX_LANES * CHART_NOTES_MAX, track->fx_notes_program);

    // return the track
    return track;
}

void track_free(Track *track)
{
    // free all the programs
    program_free(track->lane_program);
    program_free(track->measure_bars_program);
    program_free(track->bt_notes_program);
    program_free(track->fx_notes_program);

    // free all the meshes
    mesh_free(track->lane_mesh);
    mesh_free(track->measure_bars_mesh);
    mesh_free(track->bt_notes_mesh);
    mesh_free(track->fx_notes_mesh);

    // free the track
    free(track);
}

void track_draw(Track *track, double time, double speed)
{
    // update the track speed and tempo
    bool reload = false;

    if (track->speed != speed)
    {
        track->speed = speed;

        // reload the track meshes when speed changes
        // todo: this is not an acceptable solution, but works for testing
        // reloading everything on speed change is much too costly
        // loading should be buffered, in that it only loads x measures that are visible then deletes/loads new ones on scroll
        // then these reload events only have to reload a small portion of the measures and new ones will be created with the params
        reload_meshes(track);
    }

    // update the current tempo
    for (int i = 0; i < track->chart->num_tempos; i++)
    {
        if (track->chart->tempos[i].time > time)
            break;

        track->tempo_index = i;
    }

    // calculate the subbeat of time
    double time_subbeat = time_to_subbeat(track->chart, track->tempo_index, time);

    // create the projection matrix
    mat4_t projection = m4_perspective(90.0f,
                                       (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
                                       0.1f,
                                       TRACK_LENGTH + -TRACK_CAMERA_OFFSET); //clip past the end of the track

    // create the view matrix
    mat4_t view = m4_look_at(vec3(0, 0, -TRACK_LENGTH + TRACK_CAMERA_OFFSET),
                             vec3(0, TRACK_VISUAL_OFFSET, 0),
                             vec3(0, 1, 0));

    view = m4_mul(view, m4_rotation_x(85.0f / 180.0f * M_PI));

    // create the model matrix
    mat4_t model = m4_identity();
    model = m4_mul(model, m4_translation(vec3(0, -TRACK_LENGTH, 0))); //move the track so the end point is at 0,0,0

    // draw the lane
    program_use(track->lane_program);
    program_set_matrices(track->lane_program, projection, view, model);
    mesh_draw(track->lane_mesh);

    // scroll the bars and notes
    // subtract track_length so 0 scroll is at the start of the track, not the middle
    model = m4_mul(model, m4_translation(vec3(0, -subbeat_position(time_subbeat, speed) - TRACK_LENGTH, 0)));

    // draw the bars and notes above the track
    // todo: theres probably a better way to do this
    model = m4_mul(model, m4_translation(vec3(0, 0, -0.01f)));

    // draw the measure bars
    program_use(track->measure_bars_program);
    program_set_matrices(track->measure_bars_program, projection, view, model);
    mesh_draw(track->measure_bars_mesh);

    // draw the bt notes
    model = m4_mul(model, m4_translation(vec3(0, 0, -0.02f)));
    program_use(track->bt_notes_program);
    program_set_matrices(track->bt_notes_program, projection, view, model);
    mesh_draw(track->bt_notes_mesh);

    // draw the fx notes
    model = m4_mul(model, m4_translation(vec3(0, 0, 0.01f)));
    program_use(track->fx_notes_program);
    program_set_matrices(track->fx_notes_program, projection, view, model);
    mesh_draw(track->fx_notes_mesh);
}
