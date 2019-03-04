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

    // create the beam program
    track->beam_program = program_create("beam.vs", "beam.fs", true);
    track->uniform_beam_judgement_id = program_get_uniform_id(track->beam_program, "judgement");
    track->uniform_beam_alpha_id = program_get_uniform_id(track->beam_program, "alpha");

    // create the bt beam mesh
    create_beam_mesh(track->beam_program,
                     &track->bt_beam_mesh,
                     BT_MESH_NOTE_WIDTH);

    // create the fx beam mesh
    create_beam_mesh(track->beam_program,
                     &track->fx_beam_mesh,
                     FX_MESH_NOTE_WIDTH);

    // create the measure bar, bt, fx, and analog meshes
    track->measure_bar_mesh = measure_bar_mesh_create(chart);
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
    program_free(track->beam_program);

    // free all the meshes
    mesh_free(track->lane_mesh);
    mesh_free(track->bt_beam_mesh);
    mesh_free(track->fx_beam_mesh);

    // free all the measure bar/note/analog meshes
    measure_bar_mesh_free(track->measure_bar_mesh);
    note_mesh_free(track->bt_mesh);
    note_mesh_free(track->fx_mesh);
    analog_mesh_free(track->analog_mesh);

    // free the track
    free(track);
}

float track_beat_size()
{
    // return the draw size of a beat on the track at 1x speed
    return 1.0f / TRACK_BEAT_SPEED * TRACK_LENGTH;
}

float track_subbeat_position(double subbeat)
{
    // return the draw position of the given subbeat on the track at the 1x speed
    return subbeat / CHART_BEAT_SUBBEATS * track_beat_size();
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

void track_draw(Track *track, int tempo_index, double subbeat, double speed)
{
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
    mat4_t scrolled_model = m4_mul(model, m4_translation(vec3(0, -(track_subbeat_position(subbeat) * speed) - (TRACK_LENGTH / 2), 0)));

    // get the start and end subbeat of the given subbeat for drawing
    int subbeats_per_track = ceil(TRACK_LENGTH / (track_beat_size() * speed)) * CHART_BEAT_SUBBEATS;
    int extra_subbeats = CHART_BEAT_SUBBEATS * 2;
    uint16_t start_subbeat = subbeat - extra_subbeats;
    uint16_t end_subbeat = subbeat + subbeats_per_track + extra_subbeats;

    // draw the measure bars
    measure_bar_mesh_draw(track->measure_bar_mesh, projection, view, scrolled_model, start_subbeat, end_subbeat, speed);

    // normal blending for beams
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // draw the bt beams
    draw_beams(track,
               CHART_BT_LANES,
               track->bt_beam_states,
               projection,
               view,
               model,
               track->bt_beam_mesh,
               BT_MESH_NOTE_WIDTH,
               TRACK_BT_BEAM_ALPHA);

    // draw the fx beams
    draw_beams(track,
               CHART_FX_LANES,
               track->fx_beam_states,
               projection,
               view,
               model,
               track->fx_beam_mesh,
               FX_MESH_NOTE_WIDTH,
               TRACK_FX_BEAM_ALPHA);

    // draw all the notes and analogs in order
    note_mesh_draw_holds(track->fx_mesh, projection, view, scrolled_model, start_subbeat, end_subbeat, speed); //fx holds
    note_mesh_draw_holds(track->bt_mesh, projection, view, scrolled_model, start_subbeat, end_subbeat, speed); //bt holds
    analog_mesh_draw(track->analog_mesh, projection, view, scrolled_model, start_subbeat, end_subbeat, speed); //analogs
    note_mesh_draw_chips(track->fx_mesh, projection, view, scrolled_model, start_subbeat, end_subbeat, speed); //fx chips
    note_mesh_draw_chips(track->bt_mesh, projection, view, scrolled_model, start_subbeat, end_subbeat, speed); //bt chip
}
