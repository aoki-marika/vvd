#include "track.h"

#include <stdlib.h>
#include <math.h>

#define MATH_3D_IMPLEMENTATION
#include <arkanis/math_3d.h>

#include "screen.h"
#include "note_utils.h"

Track *track_create(Chart *chart)
{
    // create the track
    Track *track = malloc(sizeof(Track));

    // set the track properties
    track->chart = chart;
    track->tempo_index = 0;
    track->speed = 1.0;
    track->buffer_position = -1;

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

    // create the bt chips and holds programs and meshes
    track->bt_chips_program = program_create("bt_chip.vs", "bt_chip.fs", true);
    track->bt_holds_program = program_create("bt_hold.vs", "bt_hold.fs", true);

    size_t bt_mesh_size = MESH_VERTICES_QUAD * CHART_BT_LANES * CHART_NOTES_MAX;
    track->bt_chips_mesh = mesh_create(bt_mesh_size, track->bt_chips_program, GL_DYNAMIC_DRAW);
    track->bt_holds_mesh = mesh_create(bt_mesh_size, track->bt_holds_program, GL_DYNAMIC_DRAW);

    // create the bt lanes vertices
    for (int i = 0; i < CHART_BT_LANES; i++)
        track->bt_lanes_vertices[i] = malloc(sizeof(TrackLaneVertices));

    // create the fx chips and holds programs and meshes
    track->fx_chips_program = program_create("fx_chip.vs", "fx_chip.fs", true);
    track->fx_holds_program = program_create("fx_hold.vs", "fx_hold.fs", true);

    size_t fx_mesh_size = MESH_VERTICES_QUAD * CHART_FX_LANES * CHART_NOTES_MAX;
    track->fx_chips_mesh = mesh_create(fx_mesh_size, track->fx_chips_program, GL_DYNAMIC_DRAW);
    track->fx_holds_mesh = mesh_create(fx_mesh_size, track->fx_holds_program, GL_DYNAMIC_DRAW);

    // create the fx lanes vertices
    for (int i = 0; i < CHART_FX_LANES; i++)
        track->fx_lanes_vertices[i] = malloc(sizeof(TrackLaneVertices));

    // return the track
    return track;
}

void track_free(Track *track)
{
    // free all the programs
    program_free(track->lane_program);
    program_free(track->measure_bars_program);
    program_free(track->bt_chips_program);
    program_free(track->bt_holds_program);
    program_free(track->fx_chips_program);
    program_free(track->fx_holds_program);

    // free all the meshes
    mesh_free(track->lane_mesh);
    mesh_free(track->measure_bars_mesh);
    mesh_free(track->bt_chips_mesh);
    mesh_free(track->bt_holds_mesh);
    mesh_free(track->fx_chips_mesh);
    mesh_free(track->fx_holds_mesh);

    // free all the allocated properties
    for (int i = 0; i < CHART_BT_LANES; i++)
        free(track->bt_lanes_vertices[i]);

    for (int i = 0; i < CHART_FX_LANES; i++)
        free(track->fx_lanes_vertices[i]);

    // free the track
    free(track);
}

float beat_size(double speed)
{
    // return the draw size of a beat on the track at the given speed
    return speed / TRACK_BEAT_SPEED * TRACK_LENGTH;
}

float subbeat_position(double subbeat, double speed)
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
        float start_position = subbeat_position((i - 1) * 4 * CHART_BEAT_SUBBEATS, track->speed);
        float end_position = subbeat_position(i * 4 * CHART_BEAT_SUBBEATS, track->speed);

        // get the size of the current measure as if it was 4/4 time by getting the difference between the start and end
        // then multiply by numerator/denominator to get the final size of the measure
        Beat *beat = &track->chart->beats[beat_index];
        float measure_size = (end_position - start_position) * ((float)beat->numerator / (float)beat->denominator);

        // increment measure_position.y for the next measure
        measure_position.y += measure_size;
    }
}

void update_notes_mesh(Mesh *chips_mesh, //the mesh to add chip note vertices to
                       Mesh *holds_mesh, //the mesh to add hold note vertices to
                       uint16_t start_subbeat, //the minimum subbeat for notes to add
                       uint16_t end_subbeat, // the maximum subbeat for notes to add
                       int num_lanes, //the number of lanes for the given notes type
                       int num_notes[num_lanes], //the number of notes per lane for the given notes type
                       Note *notes[num_lanes], //the notes to potentially add
                       TrackLaneVertices *lanes_vertices[num_lanes], //the lane vertices to update for the given notes type
                       float note_width, //the width of the given notes type
                       double speed) // the current scroll speed
{
    // for each lane
    for (int l = 0; l < num_lanes; l++)
    {
        // get the current lane vertices
        TrackLaneVertices *lane_vertices = lanes_vertices[l];

        // reset the offset and size
        lane_vertices->offset = 0;
        lane_vertices->size = 0;

        // whether or not the offset and size for the current lane has been set
        bool offset_set = false;
        bool size_set = false;

        // for each note
        for (int n = 0; n < num_notes[l]; n++)
        {
            Note *note = &notes[l][n];

            // get the index of the vertices for the current note
            int vertices_index = ((l * CHART_NOTES_MAX) + n) * MESH_VERTICES_QUAD;

            // if offset hasnt been set yet
            if (!offset_set)
            {
                // set offset
                lane_vertices->offset = vertices_index;

                // skip creating the note vertices if it is before the current subbeat
                if ((note->hold ? note->end_subbeat : note->start_subbeat) < start_subbeat)
                    continue;
                // all before start subbeat notes have been skipped, mark the offset as set
                else
                    offset_set = true;
            }

            // if size hasnt been set yet
            if (!size_set)
            {
                // set size
                lane_vertices->size = vertices_index - lane_vertices->offset;

                // last notes end up getting skipped as vertices_index isnt incremented, so do it manually
                // todo: better way of doing this?
                if (n == num_notes[l] - 1)
                    lane_vertices->size += MESH_VERTICES_QUAD;

                // break this lane if the current note is after the end subbeat
                // no notes after it can be within range
                if (note->start_subbeat >= end_subbeat)
                    break;
            }

            // calculate the start position of the note
            vec3_t position = vec3((l * note_width) - (TRACK_NOTES_WIDTH / 2),
                                   subbeat_position(note->start_subbeat, speed),
                                   0);

            // calculate the size of the note
            vec3_t size = vec3(note_width,
                               TRACK_CHIP_HEIGHT,
                               0);

            // if this is a hold note then resize it from the start to end positions
            if (note->hold)
                size.y = subbeat_position(note->end_subbeat, speed) - position.y;

            // add the note vertices to the mesh
            mesh_set_vertices_quad(note->hold ? holds_mesh : chips_mesh,
                                   vertices_index,
                                   size.x, size.y,
                                   position);
        }
    }
}

void update_chart_meshes(Track *track, int buffer_position, int num_chunks)
{
    // calculate the start and end subbeat for buffer_position
    uint16_t start_subbeat = buffer_position * TRACK_BUFFER_CHUNK_BEATS * CHART_BEAT_SUBBEATS;
    uint16_t end_subbeat = (buffer_position + num_chunks) * TRACK_BUFFER_CHUNK_BEATS * CHART_BEAT_SUBBEATS;

    // update the bt notes
    update_notes_mesh(track->bt_chips_mesh,
                      track->bt_holds_mesh,
                      start_subbeat,
                      end_subbeat,
                      CHART_BT_LANES,
                      track->chart->num_bt_notes,
                      track->chart->bt_notes,
                      track->bt_lanes_vertices,
                      TRACK_BT_WIDTH,
                      track->speed);

    // update the fx notes
    update_notes_mesh(track->fx_chips_mesh,
                      track->fx_holds_mesh,
                      start_subbeat,
                      end_subbeat,
                      CHART_FX_LANES,
                      track->chart->num_fx_notes,
                      track->chart->fx_notes,
                      track->fx_lanes_vertices,
                      TRACK_BT_WIDTH * (CHART_BT_LANES / CHART_FX_LANES),
                      track->speed);
}

void update_chart_meshes_at_time(Track *track, double time, bool force)
{
    // get the beat of time
    double time_beat = time_to_subbeat(track->chart, track->tempo_index, time) / CHART_BEAT_SUBBEATS;

    // get the buffer position for time_beat
    // subtract buffer so buffer position starts at the beginning of the buffer
    int buffer_position = ceil(time_beat / TRACK_BUFFER_CHUNK_BEATS) - TRACK_EXTRA_BUFFER_CHUNKS;

    // ensure buffer position is always at least zero
    if (buffer_position < 0)
        buffer_position = 0;

    // if the buffer position changed or update should be forced
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

        // update the chart meshes
        update_chart_meshes(track, buffer_position, num_chunks);
    }
}

void track_set_speed(Track *track, double speed)
{
    track->speed = speed;

    // reload the measure bars mesh
    load_measure_bars_mesh(track);

    // update the chart meshes
    update_chart_meshes_at_time(track, track->time, true);
}

void draw_lanes(Program *program,
                Mesh *mesh,
                mat4_t projection, mat4_t view, mat4_t model,
                int num_lanes,
                TrackLaneVertices *lanes_vertices[num_lanes])
{
    // set the programs mvp matrices
    program_use(program);
    program_set_matrices(program, projection, view, model);

    // draw each lane
    for (int i = 0; i < num_lanes; i++)
    {
        TrackLaneVertices *lane_vertices = lanes_vertices[i];
        mesh_draw(mesh, lane_vertices->offset, lane_vertices->size);
    }
}

void track_draw(Track *track, double time)
{
    // set the given tracks time
    track->time = time;

    // update the current tempo
    for (int i = 0; i < track->chart->num_tempos; i++)
    {
        if (track->chart->tempos[i].time > time)
            break;

        track->tempo_index = i;
    }

    // update the chart meshes
    update_chart_meshes_at_time(track, time, false);

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

    for (int i = 0; i < CHART_ANALOG_LANES + 1; i++)
    {
        glUniform1i(track->uniform_lane_lane_id, i);
        mesh_draw(track->lane_mesh, i * MESH_VERTICES_QUAD, MESH_VERTICES_QUAD);
    }

    // scroll the bars and notes
    // subtract half of track_length so 0 scroll is at the start of the track, not 0,0,0
    double time_subbeat = time_to_subbeat(track->chart, track->tempo_index, time);
    model = m4_mul(model, m4_translation(vec3(0, -subbeat_position(time_subbeat, track->speed) - (TRACK_LENGTH / 2), 0)));

    // draw the measure bars
    program_use(track->measure_bars_program);
    program_set_matrices(track->measure_bars_program, projection, view, model);
    mesh_draw_all(track->measure_bars_mesh);

    // draw the fx holds
    draw_lanes(track->fx_holds_program,
               track->fx_holds_mesh,
               projection, view, model,
               CHART_FX_LANES,
               track->fx_lanes_vertices);

    // draw the bt holds
    draw_lanes(track->bt_holds_program,
               track->bt_holds_mesh,
               projection, view, model,
               CHART_BT_LANES,
               track->bt_lanes_vertices);

    // draw the fx chips
    draw_lanes(track->fx_chips_program,
               track->fx_chips_mesh,
               projection, view, model,
               CHART_FX_LANES,
               track->fx_lanes_vertices);

    // draw the bt chips
    draw_lanes(track->bt_chips_program,
               track->bt_chips_mesh,
               projection, view, model,
               CHART_BT_LANES,
               track->bt_lanes_vertices);
}
