#include "track.h"

#include <stdlib.h>
#include <math.h>
#include <inttypes.h>

#define MATH_3D_IMPLEMENTATION
#include <arkanis/math_3d.h>

#include "screen.h"

// could maybe get time measure by caching the length of the chart (combine all bpm durations?)
// then divide that by time to get % and do something to convert that to measure using measure size

int note_to_subbeat_end(uint16_t measure, uint8_t beat, uint8_t subbeat, Beat *current_beat)
{
    // return the amount of subbeats between the given beat and note
    return ((measure - current_beat->measure) * current_beat->numerator + beat) * (CHART_BEAT_MAX_SUBBEATS * 4 / current_beat->denominator) + subbeat;
}

int note_to_subbeat(Chart *chart, uint16_t measure, uint8_t beat, uint8_t subbeat)
{
    int total_subbeats = 0;

    // for each beat
    for (int i = 0; i < chart->num_beats; i++)
    {
        // get the current beat
        Beat *current_beat = &chart->beats[i];

        // if this is the last beat
        if (i == chart->num_beats - 1)
        {
            total_subbeats += note_to_subbeat_end(measure, beat, subbeat, current_beat);
        }
        // if this is not the last beat
        else
        {
            // get the next beat
            Beat *next_beat = &chart->beats[i + 1];

            // if the next beat applies to the given measure
            if (measure >= next_beat->measure)
            {
                // append the amount of subbeats between the current and next beats to total_subbeats
                total_subbeats += ((next_beat->measure - current_beat->measure) * current_beat->numerator) * (CHART_BEAT_MAX_SUBBEATS * 4 / current_beat->denominator);
            }
            // if the next beat doesnt apply to the given measure
            else
            {
                total_subbeats += note_to_subbeat_end(measure, beat, subbeat, current_beat);
                break;
            }
        }
    }

    return total_subbeats;
}

float position_by_subbeat(int subbeat, double speed)
{
    return (float)subbeat / CHART_BEAT_MAX_SUBBEATS * speed / TRACK_BEAT_SPEED * (TRACK_LENGTH * 2);
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

            // get the subbeat the note starts on
            int start_subbeat = note_to_subbeat(chart,
                                                note->start_measure,
                                                note->start_beat,
                                                note->start_subbeat);

            // calculate the position of the note
            int lane = abs(l - (num_lanes - 1)); //lanes are rendered mirrored for some reason
            vec3_t position = vec3((lane * width) - (track_size.x / 2),
                                   position_by_subbeat(start_subbeat, speed),
                                   0);

            // calculate the size of the note
            vec3_t size = vec3(width,
                               TRACK_CHIP_HEIGHT,
                               0);

            if (note->hold)
            {
                // get the length of the hold in subbeats
                int length_subbeats = note_to_subbeat(chart,
                                                      note->end_measure,
                                                      note->end_beat,
                                                      note->end_subbeat);

                // resize the note
                size.y = position_by_subbeat(length_subbeats, speed) - position.y;
            }

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
    int beat_index = 0;
    int total_beats = 0;

    // for each measure
    for (uint16_t m = 0; m < track->chart->end_measure + 1; m++)
    {
        // increment the current beat if the next one is after the current measure
        if (beat_index < track->chart->num_beats - 1 &&
            m >= track->chart->beats[beat_index + 1].measure)
            beat_index++;

        // increment total_beats using the current beats numerator
        total_beats += track->chart->beats[beat_index].numerator;
    }

    // calculate reused values
    vec3_t track_size = vec3(TRACK_WIDTH * 2, TRACK_LENGTH * 2, 0);

    // draw the beat/measure lines
    for (int i = 0; i < total_beats; i++)
    {
        // calculate the position and size of the bar
        vec3_t position = vec3(-track_size.x / 2, position_by_subbeat(i * CHART_BEAT_MAX_SUBBEATS, track->speed), 0);
        vec3_t size = vec3(track_size.x, TRACK_BAR_HEIGHT, 0);

        // get the respective mesh and offset for the current bar
        Mesh *mesh;
        int offset;

        // the first and every 4th beat is a measure bar, all others are beats
        if (i % 4 == 0 || i == 0)
        {
            mesh = track->measure_bars_mesh;
            offset = (i / 4) * MESH_VERTICES_QUAD;
        }
        else
        {
            mesh = track->beat_bars_mesh;
            offset = i * MESH_VERTICES_QUAD;
        }

        // create the bar
        mesh_set_vertices_quad_pos(mesh,
                                   offset,
                                   size.x, size.y,
                                   position);
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

double time_for_subbeat_at_tempo(int subbeat, Tempo *tempo)
{
    return ((double)subbeat / CHART_BEAT_MAX_SUBBEATS) * (60.0f / tempo->bpm) * 1000.0;
}

void cache_tempo_times(Track *track)
{
    for (int i = 0; i < track->chart->num_tempos; i++)
    {
        Tempo *tempo = &track->chart->tempos[i];
        track->tempo_subbeats[i] = note_to_subbeat(track->chart, tempo->measure, tempo->beat, tempo->subbeat);
    }

    double total_duration = 0;

    for (int i = 0; i < track->chart->num_tempos; i++)
    {
        track->tempo_times[i] = total_duration;

        int subbeat = track->tempo_subbeats[i];
        int next_subbeat;

        if (i + 1 < track->chart->num_tempos)
            next_subbeat = track->tempo_subbeats[i + 1];
        else
            next_subbeat = track->end_subbeat;

        Tempo *tempo = &track->chart->tempos[i];
        double duration = time_for_subbeat_at_tempo(next_subbeat - subbeat, tempo);
        total_duration += duration;
    }
}

Track *track_create(Chart *chart)
{
    // create the track
    Track *track = malloc(sizeof(Track));

    // set the track properties
    track->chart = chart;
    track->tempo_times = malloc(CHART_EVENTS_MAX * sizeof(double));
    track->tempo_subbeats = malloc(CHART_EVENTS_MAX * sizeof(double));
    track->end_subbeat = note_to_subbeat(track->chart, track->chart->end_measure, track->chart->end_beat, track->chart->end_subbeat);
    track->tempo_index = 0;
    track->speed = 0;

    // create the lane program and mesh
    track->lane_program = program_create("lane.vs", "lane.fs", true);
    track->lane_mesh = mesh_create(MESH_VERTICES_QUAD, track->lane_program);
    mesh_set_vertices_quad(track->lane_mesh, 0, TRACK_WIDTH, TRACK_LENGTH);

    // create the measure bars program and mesh
    track->measure_bars_program = program_create("measure_bar.vs", "measure_bar.fs", true);
    track->measure_bars_mesh = mesh_create(MESH_VERTICES_QUAD * TRACK_MEASURE_BARS_MAX, track->measure_bars_program);

    // create the beat bars program and mesh
    track->beat_bars_program = program_create("beat_bar.vs", "beat_bar.fs", true);
    track->beat_bars_mesh = mesh_create(MESH_VERTICES_QUAD * TRACK_BEAT_BARS_MAX * TRACK_MEASURE_BARS_MAX, track->beat_bars_program);

    // create the bt notes program and mesh
    track->bt_notes_program = program_create("bt_note.vs", "bt_note.fs", true);
    track->bt_notes_mesh = mesh_create(MESH_VERTICES_QUAD * CHART_BT_LANES * CHART_NOTES_MAX, track->bt_notes_program);

    // create the fx notes program and mesh
    track->fx_notes_program = program_create("fx_note.vs", "fx_note.fs", true);
    track->fx_notes_mesh = mesh_create(MESH_VERTICES_QUAD * CHART_FX_LANES * CHART_NOTES_MAX, track->fx_notes_program);

    // cache the tempo times
    cache_tempo_times(track);

    // return the track
    return track;
}

void track_free(Track *track)
{
    // free all the programs
    program_free(track->lane_program);
    program_free(track->measure_bars_program);
    program_free(track->beat_bars_program);
    program_free(track->bt_notes_program);
    program_free(track->fx_notes_program);

    // free all the meshes
    mesh_free(track->lane_mesh);
    mesh_free(track->measure_bars_mesh);
    mesh_free(track->beat_bars_mesh);
    mesh_free(track->bt_notes_mesh);
    mesh_free(track->fx_notes_mesh);

    // free all the allocated properties
    free(track->tempo_times);
    free(track->tempo_subbeats);

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
        if (track->tempo_times[i] > time)
            break;

        track->tempo_index = i;
    }

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
    // todo: proper scrolling calculations
    // tempo = track->chart->tempos[track->tempo_index]
    // time = track->tempo_times[track->tempo_index]
    // scroll += time / 60000 * tempo->bpm * speed * ((TRACK_LENGTH * 2) / TRACK_BEAT_SPEED))

    // get the tempo and start time
    Tempo *tempo = &track->chart->tempos[track->tempo_index];
    double start_time = track->tempo_times[track->tempo_index];

    // get the end time
    double end_time;
    if (track->tempo_index + 1 < track->chart->num_tempos)
        end_time = track->tempo_times[track->tempo_index + 1];
    else
    {
        double duration = time_for_subbeat_at_tempo(track->end_subbeat - track->tempo_subbeats[track->tempo_index], tempo);
        end_time = start_time + duration;
    }

    // calculate the percentage of the current tempo the track should be scrolled to
    float percent = (time - start_time) / (end_time - start_time);

    // get the current tempo position
    int start_subbeat = track->tempo_subbeats[track->tempo_index];
    float start_position = position_by_subbeat(start_subbeat, speed);

    // get the next tempo position
    int end_subbeat;
    if (track->tempo_index + 1 < track->chart->num_tempos)
        end_subbeat = track->tempo_subbeats[track->tempo_index + 1];
    else
        end_subbeat = track->end_subbeat;

    float end_position_offset = position_by_subbeat(end_subbeat - start_subbeat, speed);

    // scroll the chart
    float scroll = -(start_position + (end_position_offset * percent));

    // !!!!!!! WORKS !!!!!!!
    // except for the end bpm

    // subtract track_length so 0 scroll is at the start of the track, not the middle
    model = m4_mul(model, m4_translation(vec3(0, scroll - TRACK_LENGTH, 0)));

    // draw the bars and notes above the track
    // todo: theres probably a better way to do this
    model = m4_mul(model, m4_translation(vec3(0, 0, -0.01f)));

    // draw the measure bars
    program_use(track->measure_bars_program);
    program_set_matrices(track->measure_bars_program, projection, view, model);
    mesh_draw(track->measure_bars_mesh);

    // draw the beat bars
    program_use(track->beat_bars_program);
    program_set_matrices(track->beat_bars_program, projection, view, model);
    mesh_draw(track->beat_bars_mesh);

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
