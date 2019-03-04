#include "measure_bar_mesh.h"

#include <stdlib.h>

#include "track.h"

void load_measure_bars(MeasureBarMesh *mesh)
{
    int beat_index = 0;
    float measure_position = 0;
    int measure_subbeat = 0;

    for (int i = 0; i < mesh->chart->num_measures; i++)
    {
        // set the current measures position
        mesh->measure_bar_positions[i] = measure_position;
        mesh->measure_bar_subbeats[i] = measure_subbeat;

        // increment the current beat if the next beat is at or after the current measure
        if (beat_index + 1 < mesh->chart->num_beats &&
            i >= mesh->chart->beats[beat_index + 1].measure)
            beat_index++;

        // get the start and end position of this measure, as if it was in 4/4 time
        float start_position = track_subbeat_position((i - 1) * 4 * CHART_BEAT_SUBBEATS);
        float end_position = track_subbeat_position(i * 4 * CHART_BEAT_SUBBEATS);

        // get the size of the current measure as if it was 4/4 time by getting the difference between the start and end
        // then multiply by numerator/denominator to get the final size of the measure
        Beat *beat = &mesh->chart->beats[beat_index];
        float measure_size = (end_position - start_position) * ((float)beat->numerator / (float)beat->denominator);

        // get the number of subbeats in the current measure by converting the measure size to subbeats
        int measure_subbeats = measure_size / track_beat_size() * CHART_BEAT_SUBBEATS;

        // increment the measure position and subbeat for the next measure
        measure_position += measure_size;
        measure_subbeat += measure_subbeats;
    }
}

MeasureBarMesh *measure_bar_mesh_create(Chart *chart)
{
    // create the measure bar mesh
    MeasureBarMesh *mesh = malloc(sizeof(MeasureBarMesh));

    // set the meshes properties
    mesh->chart = chart;
    mesh->measure_bar_positions = malloc(chart->num_measures * sizeof(float));
    mesh->measure_bar_subbeats = malloc(chart->num_measures * sizeof(int));

    // create the program
    mesh->program = program_create("measure_bar.vs", "measure_bar.fs", true);
    mesh->uniform_speed_id = program_get_uniform_id(mesh->program, "speed");
    mesh->uniform_position_id = program_get_uniform_id(mesh->program, "position");

    // create the mesh
    mesh->mesh = mesh_create(MEASURE_BAR_MESH_MEASURE_BAR_SIZE, mesh->program, GL_STATIC_DRAW);
    mesh_set_vertices_quad(mesh->mesh,
                           0,
                           TRACK_WIDTH,
                           TRACK_BAR_HEIGHT,
                           vec3(-TRACK_WIDTH / 2, 0, 0));

    // load the measure bars
    load_measure_bars(mesh);

    // return the mesh
    return mesh;
}

void measure_bar_mesh_free(MeasureBarMesh *mesh)
{
    free(mesh->measure_bar_positions);
    free(mesh->measure_bar_subbeats);
    free(mesh);
}

void measure_bar_mesh_draw(MeasureBarMesh *mesh,
                           mat4_t projection,
                           mat4_t view,
                           mat4_t model,
                           uint16_t start_subbeat,
                           uint16_t end_subbeat,
                           double speed)
{
    // normal blending for measure bars
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // draw the measure bars
    program_use(mesh->program);
    program_set_matrices(mesh->program, projection, view, model);
    glUniform1f(mesh->uniform_speed_id, speed);
    mesh_draw_start(mesh->mesh);

    for (int i = 0; i < mesh->chart->num_measures; i++)
    {
        // draw the current bar if its in range and not removed
        if (mesh->measure_bar_subbeats[i] >= start_subbeat &&
            mesh->measure_bar_subbeats[i] <= end_subbeat)
        {
            glUniform1f(mesh->uniform_position_id, mesh->measure_bar_positions[i]);
            mesh_draw_vertices(mesh->mesh, 0, MEASURE_BAR_MESH_MEASURE_BAR_SIZE);
        }
    }

    mesh_draw_end(mesh->mesh);
}
