#include "analog_mesh.h"

#include "track.h"

AnalogMesh *analog_mesh_create(Chart *chart)
{
    // create the mesh
    AnalogMesh *mesh = malloc(sizeof(AnalogMesh));

    // set the meshes properties
    mesh->chart = chart;

    // create the program
    mesh->program = program_create("analog.vs", "analog.fs", true);
    mesh->program_lane_id = program_get_uniform_id(mesh->program, "lane");

    // create the meshes
    // todo: analog mesh sizes do not need to be this big, see note_mesh.c
    size_t mesh_size = CHART_NOTES_MAX * CHART_ANALOG_POINTS_MAX * MESH_VERTICES_QUAD;
    for (int i = 0; i < CHART_ANALOG_LANES; i++)
        mesh->lane_meshes[i] = mesh_create(mesh_size, mesh->program, GL_DYNAMIC_DRAW);

    // return the mesh
    return mesh;
}

void analog_mesh_free(AnalogMesh *mesh)
{
    // free the program
    program_free(mesh->program);

    // free the lane meshes
    for (int i = 0; i < CHART_ANALOG_LANES; i++)
        mesh_free(mesh->lane_meshes[i]);

    // free the mesh
    free(mesh);
}

float analog_point_draw_position(AnalogPoint *point)
{
    // return the draw position, on the x axis, of the given point
    float analog_track_width = (TRACK_WIDTH - TRACK_ANALOG_WIDTH) * point->position_scale;
    return point->position * analog_track_width - (TRACK_ANALOG_WIDTH / 2) - (analog_track_width / 2);
}

void create_segment_vertices(Mesh *mesh,
                             int *vertices_index,
                             AnalogPoint *start_point,
                             AnalogPoint *end_point,
                             double speed)
{
    // get the segments start position
    vec3_t start_position = vec3(analog_point_draw_position(start_point),
                                 track_subbeat_position(start_point->subbeat, speed),
                                 0);

    // offset the start of the segment if the last segment was a slam, so the vertices arent overlaying eachother
    if (start_point->slam)
        start_position.y += track_subbeat_position(TRACK_ANALOG_SLAM_SUBBEATS, speed);

    // get the segments end position
    vec3_t end_position = vec3(analog_point_draw_position(end_point),
                               track_subbeat_position(end_point->subbeat, speed),
                               0);

    // create the vertices
    mesh_set_vertices_quad_edges(mesh,
                                 *vertices_index,
                                 TRACK_ANALOG_WIDTH,
                                 start_position,
                                 end_position);

    // increment vertices_index
    *vertices_index += MESH_VERTICES_QUAD;
}

void create_slam_vertices(Mesh *mesh,
                          int *vertices_index,
                          AnalogPoint *start_point,
                          AnalogPoint *end_point,
                          bool last_segment,
                          double speed)
{
    // get the slams position
    vec3_t position = vec3(0,
                           track_subbeat_position(start_point->subbeat, speed),
                           0);

    // get the slams x position depending on which point is closer to 0
    if (start_point->position < end_point->position)
        position.x = analog_point_draw_position(start_point);
    else
        position.x = analog_point_draw_position(end_point);

    // get the slams width by getting the difference in draw positions between the start and end points
    // + TRACK_ANALOG_WIDTH so the edges of the slam go to the edges of other segments
    float width = fabs(analog_point_draw_position(end_point) - analog_point_draw_position(start_point)) + TRACK_ANALOG_WIDTH;

    // create the slam vertices
    mesh_set_vertices_quad(mesh,
                           *vertices_index,
                           width,
                           track_subbeat_position(TRACK_ANALOG_SLAM_SUBBEATS, speed),
                           position);

    // increment vertices_index
    *vertices_index += MESH_VERTICES_QUAD;

    // append a slam tail if this is the last segment of the analog of the given segment
    if (last_segment)
    {
        // get the tails position
        vec3_t tail_position = vec3(analog_point_draw_position(end_point),
                                    position.y + track_subbeat_position(TRACK_ANALOG_SLAM_SUBBEATS, speed),
                                    0);

        // create the tail vertices
        mesh_set_vertices_quad(mesh,
                               *vertices_index,
                               TRACK_ANALOG_WIDTH,
                               track_subbeat_position(TRACK_ANALOG_SLAM_SUBBEATS, speed),
                               tail_position);

        // increment vertices_index
        *vertices_index += MESH_VERTICES_QUAD;
    }
}

void analog_mesh_load(AnalogMesh *mesh, uint16_t start_subbeat, uint16_t end_subbeat, double speed)
{
    for (int l = 0; l < CHART_ANALOG_LANES; l++)
    {
        // get the mesh for the current lane
        Mesh *lane_mesh = mesh->lane_meshes[l];

        // the offset for the current segments vertices
        int vertices_index = 0;

        for (int a = 0; a < mesh->chart->num_analogs[l]; a++)
        {
            Analog *analog = &mesh->chart->analogs[l][a];

            // whether or not this lane is finished creating segment vertices
            bool lane_finished = false;

            for (int p = 0; p < analog->num_points - 1; p++)
            {
                AnalogPoint *start_point = &analog->points[p];
                AnalogPoint *end_point = &analog->points[p + 1];

                // skip segments that are before start_subbeat
                if (end_point->subbeat < start_subbeat)
                    continue;

                // break this lane if this segment is after end_subbeat
                // no segments after it can be before end_subbeat
                if (start_point->subbeat > end_subbeat)
                {
                    lane_finished = true;
                    break;
                }

                // create the vertices for the current segment
                if (end_point->slam)
                    create_slam_vertices(lane_mesh,
                                         &vertices_index,
                                         start_point,
                                         end_point,
                                         p == analog->num_points - 2,
                                         speed);
                else
                    create_segment_vertices(lane_mesh,
                                            &vertices_index,
                                            start_point,
                                            end_point,
                                            speed);
            }

            // break this lane if this lane is finished creating segment vertices
            if (lane_finished)
                break;
        }

        // set the current lanes size on the given mesh
        mesh->lane_sizes[l] = vertices_index;
    }
}

void analog_mesh_draw(AnalogMesh *mesh, mat4_t projection,mat4_t view, mat4_t model)
{
    // additive blending for analogs
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // raise analogs slightly above the track
    model = m4_mul(model, m4_translation(vec3(0, 0, -0.005)));

    // draw the analogs
    program_use(mesh->program);
    program_set_matrices(mesh->program, projection, view, model);

    for (int i = 0; i < CHART_ANALOG_LANES; i++)
    {
        glUniform1i(mesh->program_lane_id, i);
        mesh_draw(mesh->lane_meshes[i], 0, mesh->lane_sizes[i]);
    }
}
