#include "track.h"

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <assert.h>
#include <math.h>

#define MATH_3D_IMPLEMENTATION
#include <arkanis/math_3d.h>

#include "screen.h"

bool time_is_after(uint16_t lhs_measure, uint8_t lhs_beat, uint8_t lhs_subbeat,
                   uint16_t rhs_measure, uint8_t rhs_beat, uint8_t rhs_subbeat)
{
    return lhs_measure > rhs_measure ||
           lhs_measure == rhs_measure && lhs_beat > rhs_beat ||
           lhs_measure == rhs_measure && lhs_beat == rhs_beat && lhs_subbeat > rhs_subbeat;
}

double time_difference(Beat *beat,
                       Tempo *tempo,
                       uint16_t lhs_measure, uint8_t lhs_beat,
                       uint16_t rhs_measure, uint8_t rhs_beat)
{
    // get the difference in beats
    int num_beats = 0;
    num_beats += (lhs_measure - rhs_measure) * beat->nominator;
    num_beats += lhs_beat - rhs_beat;

    // convert and return the difference in milliseconds
    // minutes * 60,000ms (60s, 1min) to get ms
    return (num_beats / tempo->bpm) * 60000;
}

Beat *beat_for_time(Chart *chart, uint16_t measure, uint8_t beat, uint8_t subbeat)
{
    // find the first beat at or before the given time
    Beat *last_beat;

    // for each beat
    for (int i = 0; i < chart->num_beats; i++)
    {
        // get the current beat
        Beat *current_beat = &chart->beats[i];

        // break if the current beat is after the given time
        if (time_is_after(current_beat->measure, current_beat->beat, current_beat->subbeat,
                          measure, beat, subbeat))
            break;

        // set last_beat as current_beat is now validated
        last_beat = current_beat;
    }

    // assert that a valid beat was found
    assert(last_beat);

    // return the last beat
    return last_beat;
}

Tempo *tempo_for_time(Chart *chart, uint16_t measure, uint8_t beat, uint8_t subbeat)
{
    // find the first tempo at or before the given time
    Tempo *last_tempo;

    // for each tempo
    for (int i = 0; i < chart->num_tempos; i++)
    {
        // get the current tempo
        Tempo *current_tempo = &chart->tempos[i];

        // break if the current tempo is after the given time
        if (time_is_after(current_tempo->measure, current_tempo->beat, current_tempo->subbeat,
                          measure, beat, subbeat))
            break;

        // set last_tempo as current_tempo is now validated
        last_tempo = current_tempo;
    }

    // assert that a valid tempo was found
    assert(last_tempo);

    // return the last tempo
    return last_tempo;
}

void cache_beat_times(Track *track)
{
    // the time, in ms, the last beat was at
    double last_time = 0;

    // for each beat
    for (int i = 0; i < track->chart->num_beats; i++)
    {
        // get the current beat
        Beat *beat = &track->chart->beats[i];

        // get the previous tempo, if there is one
        Beat *previous_beat = NULL;
        if (i > 0)
            previous_beat = &track->chart->beats[i - 1];

        // get the tempo for the current beat
        Tempo *tempo = tempo_for_time(track->chart, beat->measure, beat->beat, beat->subbeat);

        // get the difference between the current and previous beats
        double beat_difference = 0;
        if (previous_beat)
        {
            // get the difference if there is a previous beat
            beat_difference = time_difference(beat,
                                              tempo,
                                              beat->measure, beat->beat,
                                              previous_beat->measure, previous_beat->beat);
        }

        // increment last_time for the current beat
        last_time += beat_difference;

        // set the current beats time
        track->beat_times[i] = last_time;
    }
}

void cache_tempo_times(Track *track)
{
    // the time, in ms, the last tempo was at
    double last_time = 0;

    // for each tempo
    for (int i = 0; i < track->chart->num_tempos; i++)
    {
        // get the current tempo
        Tempo *tempo = &track->chart->tempos[i];

        // get the previous tempo, if there is one
        Tempo *previous_tempo = NULL;
        if (i > 0)
            previous_tempo = &track->chart->tempos[i - 1];

        // get the beat for the current tempo
        Beat *beat = beat_for_time(track->chart, tempo->measure, tempo->beat, tempo->subbeat);

        // get the difference between the current and previous tempos
        double tempo_difference = 0;
        if (previous_tempo)
        {
            // get the difference if there is a previous tempo
            tempo_difference = time_difference(beat,
                                               tempo,
                                               tempo->measure, tempo->beat,
                                               previous_tempo->measure, previous_tempo->beat);
        }

        // increment last_time for the current tempo
        last_time += tempo_difference;

        // set the current tempos time
        track->tempo_times[i] = last_time;
    }
}

Track *track_create(Chart *chart)
{
    // create the track
    Track *track = malloc(sizeof(Track));

    // set the track properties
    track->chart = chart;

    // allocate and cache the beat and tempo times
    track->beat_times = malloc(CHART_EVENTS_MAX * sizeof(double));
    track->tempo_times = malloc(CHART_EVENTS_MAX * sizeof(double));
    cache_beat_times(track);
    cache_tempo_times(track);

    // create the lane program and mesh
    track->lane_program = program_create("lane.vs", "lane.fs", true);
    track->lane_mesh = mesh_create(MESH_VERTICES_QUAD, track->lane_program);
    mesh_set_vertices_quad(track->lane_mesh, 0, TRACK_WIDTH, TRACK_LENGTH);

    // return the track
    return track;
}

void track_free(Track *track)
{
    // free all the programs
    program_free(track->lane_program);

    // free all the meshes
    mesh_free(track->lane_mesh);

    // free all the allocated properties
    free(track->beat_times);
    free(track->tempo_times);

    // free the track
    free(track);
}

void track_draw(Track *track, double time, double speed)
{
    // get the tempo and its time in ms for the given time
    Tempo *tempo;
    double tempo_time;

    for (int i = 0; i < track->chart->num_tempos; i++)
    {
        // break if the current tempo is after the given time
        if (track->tempo_times[i] > time)
            break;

        // set tempo and tempo_time as theyre now validated
        tempo = &track->chart->tempos[i];
        tempo_time = track->tempo_times[i];
    }

    // get the beat for the given time
    Beat *beat;

    for (int i = 0; i < track->chart->num_beats; i++)
    {
        if (track->beat_times[i] > time)
            break;

        // set beat as its now validated
        beat = &track->chart->beats[i];
    }

    // get the measure of time by adding an offset from tempos time
    // this allows this calculation to ignore previous beats/tempos
    double offset_beats = ((time - tempo_time) / 60000) * tempo->bpm;

    // todo: this may be incorrect, but seems right so far
    uint16_t time_measure = tempo->measure + (offset_beats / beat->denominator);

    // create the projection matrix
    mat4_t projection = m4_perspective(90.0f,
                                       (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
                                       0.1f,
                                       TRACK_LENGTH + TRACK_CAMERA_OFFSET); //clip past the end of the track

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
}
