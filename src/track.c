#include "track.h"

#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define MATH_3D_IMPLEMENTATION
#include <arkanis/math_3d.h>

#include "screen.h"
#include "timing.h"
#include "bt_mesh.h"
#include "fx_mesh.h"
#include "note_utils.h"
#include "interpolate.h"

void create_beam_mesh(Program *program,
                      Mesh **mesh,
                      float lane_width)
{
    // create the mesh
    *mesh = mesh_create(MESH_VERTICES_QUAD, program, GL_STATIC_DRAW);

    // set the mesh vertices for one lane
    // when rendering this lane is moved around and has its state set for each lane
    mesh_set_vertices_quad(*mesh,
                           0,
                           lane_width,
                           TRACK_LENGTH,
                           vec3(-TRACK_NOTES_WIDTH / 2,
                                -TRACK_LENGTH / 2,
                                0));
}

Track *track_create(Chart *chart)
{
    // create the track
    Track *track = malloc(sizeof(Track));

    // set the track properties
    track->chart = chart;
    track->tempo_index = 0;
    track->buffer_position = -1;

    // default the beam times so they arent triggered when the track is loaded
    double hidden_time = time_milliseconds() - TRACK_BEAM_DURATION;

    for (int i = 0; i < CHART_BT_LANES; i++)
        track->bt_beam_states[i].time = hidden_time;

    for (int i = 0; i < CHART_FX_LANES; i++)
        track->fx_beam_states[i].time = hidden_time;

    // create the lane program and mesh
    track->lane_program = program_create("lane.vs", "lane.fs", true);
    track->uniform_lane_lane_id = program_get_uniform_id(track->lane_program, "lane");
    track->lane_mesh = mesh_create(MESH_VERTICES_QUAD * (CHART_ANALOG_LANES + 1), track->lane_program, GL_STATIC_DRAW);

    // set the note lane vertices
    mesh_set_vertices_quad(track->lane_mesh,
                           0,
                           TRACK_NOTES_WIDTH,
                           TRACK_LENGTH,
                           vec3(-TRACK_NOTES_WIDTH / 2,
                                -TRACK_LENGTH / 2,
                                0));

    // create the analog gutter vertices
    for (int i = 1; i <= CHART_ANALOG_LANES; i++)
    {
        // get the position of the current lane
        vec3_t position = vec3(0, -TRACK_LENGTH / 2, 0);

        // set the x position so even lanes are on the right and uneven are on the left
        if (i % 2 == 0)
            position.x = (TRACK_NOTES_WIDTH / 2) + (TRACK_GUTTER_WIDTH * ((i - 1) / 2));
        else
            position.x = (-TRACK_NOTES_WIDTH / 2) - (TRACK_GUTTER_WIDTH * i);

        // set the gutters vertices
        mesh_set_vertices_quad(track->lane_mesh,
                               MESH_VERTICES_QUAD * i,
                               TRACK_GUTTER_WIDTH,
                               TRACK_LENGTH,
                               position);
    }

    // create the measure bars program and mesh
    track->measure_bars_program = program_create("measure_bar.vs", "measure_bar.fs", true);
    track->measure_bars_mesh = mesh_create(MESH_VERTICES_QUAD * chart->num_measures, track->measure_bars_program, GL_DYNAMIC_DRAW);

    // create the beam program
    track->beam_program = program_create("beam.vs", "beam.fs", true);
    track->uniform_beam_judgement_id = program_get_uniform_id(track->beam_program, "judgement");
    track->uniform_beam_alpha_id = program_get_uniform_id(track->beam_program, "alpha");

    // create the bt beam mesh
    create_beam_mesh(track->beam_program,
                     &track->bt_beam_mesh,
                     TRACK_BT_WIDTH);

    // create the fx beam mesh
    create_beam_mesh(track->beam_program,
                     &track->fx_beam_mesh,
                     TRACK_FX_WIDTH);

    // create the bt, fx, and analog meshes
    track->bt_mesh = bt_mesh_create(chart);
    track->fx_mesh = fx_mesh_create(chart);
    track->analog_mesh = analog_mesh_create(chart);

    // return the track
    return track;
}

void track_free(Track *track)
{
    // free all the programs
    program_free(track->lane_program);
    program_free(track->measure_bars_program);
    program_free(track->beam_program);

    // free all the meshes
    mesh_free(track->lane_mesh);
    mesh_free(track->measure_bars_mesh);
    mesh_free(track->bt_beam_mesh);
    mesh_free(track->fx_beam_mesh);

    // free all the note/analog meshes
    note_mesh_free(track->bt_mesh);
    note_mesh_free(track->fx_mesh);
    analog_mesh_free(track->analog_mesh);

    // free the track
    free(track);
}

float beat_size(double speed)
{
    // return the draw size of a beat on the track at the given speed
    return speed / TRACK_BEAT_SPEED * TRACK_LENGTH;
}

float track_subbeat_position(double subbeat, double speed)
{
    // return the draw position of the given subbeat on the track at the given speed
    return subbeat / CHART_BEAT_SUBBEATS * beat_size(speed);
}

void load_measure_bars_mesh(Track *track)
{
    // the index of the current beat in track->chart->beats
    int beat_index = 0;

    // the last measures draw position
    vec3_t measure_position = vec3(-TRACK_NOTES_WIDTH / 2, 0, 0);

    // for each measure
    for (uint16_t i = 0; i < track->chart->num_measures; i++)
    {
        // create the measure bar for the current measure
        mesh_set_vertices_quad(track->measure_bars_mesh,
                               i * MESH_VERTICES_QUAD,
                               TRACK_NOTES_WIDTH,
                               TRACK_BAR_HEIGHT,
                               measure_position);

        // increment the current beat if the next one is at or before the current measure
        if (beat_index + 1 < track->chart->num_beats &&
            i >= track->chart->beats[beat_index + 1].measure)
            beat_index++;

        // get the start and end position of this measure, as if it was in 4/4 time
        float start_position = track_subbeat_position((i - 1) * 4 * CHART_BEAT_SUBBEATS, track->speed);
        float end_position = track_subbeat_position(i * 4 * CHART_BEAT_SUBBEATS, track->speed);

        // get the size of the current measure as if it was 4/4 time by getting the difference between the start and end
        // then multiply by numerator/denominator to get the final size of the measure
        Beat *beat = &track->chart->beats[beat_index];
        float measure_size = (end_position - start_position) * ((float)beat->numerator / (float)beat->denominator);

        // increment measure_position.y for the next measure
        measure_position.y += measure_size;
    }
}

void load_chart_meshes(Track *track, int buffer_position, int num_chunks)
{
    // calculate the start and end subbeat for buffer_position
    uint16_t start_subbeat = buffer_position * TRACK_BUFFER_CHUNK_BEATS * CHART_BEAT_SUBBEATS;
    uint16_t end_subbeat = (buffer_position + num_chunks) * TRACK_BUFFER_CHUNK_BEATS * CHART_BEAT_SUBBEATS;

    // load the bt, fx, and analog meshes
    note_mesh_load(track->bt_mesh, start_subbeat, end_subbeat, track->speed);
    note_mesh_load(track->fx_mesh, start_subbeat, end_subbeat, track->speed);
    analog_mesh_load(track->analog_mesh, start_subbeat, end_subbeat, track->speed);
}

void load_chart_meshes_at_subbeat(Track *track, double subbeat, bool force)
{
    // convert subbeat to beats
    double beat = subbeat / CHART_BEAT_SUBBEATS;

    // get the buffer position for beat
    // subtract buffer so buffer position starts at the beginning of the buffer
    int buffer_position = ceil(beat / TRACK_BUFFER_CHUNK_BEATS) - TRACK_EXTRA_BUFFER_CHUNKS;

    // ensure buffer position is always at least zero
    if (buffer_position < 0)
        buffer_position = 0;

    // if the buffer position changed or load should be forced
    if (buffer_position != track->buffer_position || force)
    {
        // calculate the number of chunks that should be loaded
        //
        // the number of beats per track size at track->speed
        // divided by beats per chunk to get it in chunks
        // plus buffer * 2 for before and after buffer
        int num_chunks = ceil((TRACK_LENGTH / beat_size(track->speed)) / TRACK_BUFFER_CHUNK_BEATS) + (TRACK_EXTRA_BUFFER_CHUNKS * 2);

        // set the given tracks buffer position
        track->buffer_position = buffer_position;

        // load the chart meshes
        load_chart_meshes(track, buffer_position, num_chunks);
    }
}

void track_set_speed(Track *track, double speed)
{
    track->speed = speed;

    // reload the measure bars mesh
    load_measure_bars_mesh(track);

    // load the chart meshes
    load_chart_meshes_at_subbeat(track, track->subbeat, true);
}

void note_beam(int num_lanes,
               BeamState states[num_lanes],
               int lane,
               Judgement judgement)
{
    // assert that lane is valid
    assert(lane >= 0 && lane < num_lanes);

    // set the beam state for the given lanes properties
    BeamState *state = &states[lane];
    state->judgement = judgement;
    state->time = time_milliseconds();
}

void track_bt_beam(Track *track, int lane, Judgement judgement)
{
    // set the beam for the given lanes properties
    note_beam(CHART_BT_LANES,
              track->bt_beam_states,
              lane,
              judgement);
}

void track_fx_beam(Track *track, int lane, Judgement judgement)
{
    // set the beam for the given lanes properties
    note_beam(CHART_FX_LANES,
              track->fx_beam_states,
              lane,
              judgement);
}

void draw_beams(Track *track,
                int num_lanes,
                BeamState states[num_lanes],
                mat4_t projection,
                mat4_t view,
                mat4_t model,
                Mesh *mesh,
                float lane_width,
                double start_alpha)
{
    // use the beam program
    program_use(track->beam_program);

    // get the current time in milliseconds
    double time = time_milliseconds();

    // for each lane
    for (int i = 0; i < num_lanes; i++)
    {
        // get the current lanes beams state
        BeamState *state = &states[i];

        // only draw the beam if its animation is still occurring
        if (state->time + TRACK_BEAM_DURATION >= time)
        {
            // get the current beams current alpha
            double current_alpha = interpolate(time,
                                               state->time,
                                               state->time + TRACK_BEAM_DURATION,
                                               start_alpha,
                                               0);

            // set the shaders properties
            glUniform1i(track->uniform_beam_judgement_id, state->judgement);
            glUniform1f(track->uniform_beam_alpha_id, current_alpha);

            // set the programs matrices
            program_set_matrices(track->beam_program, projection, view, model);

            // draw the current beam
            mesh_draw_all(mesh);
        }

        // move the model matrix for the next beam
        model = m4_mul(model, m4_translation(vec3(-lane_width, 0, 0)));
    }
}

void track_draw(Track *track, int tempo_index, double subbeat)
{
    // set the given tracks subbeat and tempo index
    track->subbeat = subbeat;
    track->tempo_index = tempo_index;

    // load the chart meshes
    load_chart_meshes_at_subbeat(track, subbeat, false);

    // create the projection matrix
    mat4_t projection = m4_perspective(90.0f,
                                       (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
                                       0.1f,
                                       TRACK_LENGTH + -TRACK_CAMERA_OFFSET);

    // create the view matrix
    mat4_t view = m4_look_at(vec3(0, 0, -TRACK_LENGTH + TRACK_CAMERA_OFFSET),
                             vec3(0, TRACK_VISUAL_OFFSET, 0),
                             vec3(0, 1, 0));

    view = m4_mul(view, m4_rotation_x(86.0f / 180.0f * M_PI));

    // create the model matrix
    mat4_t model = m4_identity();
    model = m4_mul(model, m4_translation(vec3(0, -TRACK_LENGTH / 2, 0))); //move the track so the end point is at 0,0,0

    // draw the lane
    program_use(track->lane_program);
    program_set_matrices(track->lane_program, projection, view, model);
    mesh_draw_start(track->lane_mesh);

    for (int i = 0; i < 1 + CHART_ANALOG_LANES; i++)
    {
        glUniform1i(track->uniform_lane_lane_id, i);
        mesh_draw_vertices(track->lane_mesh, i * MESH_VERTICES_QUAD, MESH_VERTICES_QUAD);
    }

    mesh_draw_end(track->lane_mesh);

    // scroll the bars and notes
    // subtract half of track_length so 0 scroll is at the start of the track, not 0,0,0
    mat4_t scrolled_model = m4_mul(model, m4_translation(vec3(0, -track_subbeat_position(subbeat, track->speed) - (TRACK_LENGTH / 2), 0)));

    // draw the measure bars
    program_use(track->measure_bars_program);
    program_set_matrices(track->measure_bars_program, projection, view, scrolled_model);
    mesh_draw_all(track->measure_bars_mesh);

    // draw the bt beams
    draw_beams(track,
               CHART_BT_LANES,
               track->bt_beam_states,
               projection,
               view,
               model,
               track->bt_beam_mesh,
               TRACK_BT_WIDTH,
               TRACK_BT_BEAM_ALPHA);

    // draw the fx beams
    draw_beams(track,
               CHART_FX_LANES,
               track->fx_beam_states,
               projection,
               view,
               model,
               track->fx_beam_mesh,
               TRACK_FX_WIDTH,
               TRACK_FX_BEAM_ALPHA);

    // draw all the notes and analogs in order
    note_mesh_draw_holds(track->fx_mesh, projection, view, scrolled_model); //fx holds
    note_mesh_draw_holds(track->bt_mesh, projection, view, scrolled_model); //bt holds
    analog_mesh_draw(track->analog_mesh, projection, view, scrolled_model); //analogs
    note_mesh_draw_chips(track->fx_mesh, projection, view, scrolled_model); //fx chips
    note_mesh_draw_chips(track->bt_mesh, projection, view, scrolled_model); //bt chip
}
