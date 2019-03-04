#include "analog_mesh.h"

#include "track.h"

float analog_point_draw_position(AnalogPoint *point)
{
    // return the draw position, on the x axis, of the given point
    float analog_track_width = (TRACK_WIDTH - TRACK_ANALOG_WIDTH) * point->position_scale;
    return point->position * analog_track_width - (TRACK_ANALOG_WIDTH / 2) - (analog_track_width / 2);
}

void create_segment_vertices(Mesh *mesh,
                             int *vertices_index,
                             AnalogPoint *start_point,
                             AnalogPoint *end_point)
{
    // get the segments start position
    vec3_t start_position = vec3(analog_point_draw_position(start_point),
                                 track_subbeat_position(start_point->subbeat),
                                 0);

    // offset the start of the segment if the last segment was a slam, so the vertices arent overlaying eachother
    if (start_point->slam)
        start_position.y += track_subbeat_position(TRACK_ANALOG_SLAM_SUBBEATS);

    // get the segments end position
    vec3_t end_position = vec3(analog_point_draw_position(end_point),
                               track_subbeat_position(end_point->subbeat),
                               0);

    // create the vertices
    mesh_set_vertices_quad_edges(mesh,
                                 *vertices_index,
                                 TRACK_ANALOG_WIDTH,
                                 start_position,
                                 end_position);

    // increment vertices_index
    *vertices_index += ANALOG_MESH_SEGMENT_SIZE;
}

void create_slam_vertices(Mesh *mesh,
                          int *vertices_index,
                          AnalogPoint *start_point,
                          AnalogPoint *end_point,
                          bool last_segment)
{
    // get the slams position
    vec3_t position = vec3(0,
                           track_subbeat_position(start_point->subbeat),
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
                           track_subbeat_position(TRACK_ANALOG_SLAM_SUBBEATS),
                           position);

    // increment vertices_index
    *vertices_index += ANALOG_MESH_SLAM_SIZE;

    // append a slam tail if this is the last segment of the analog of the given segment
    if (last_segment)
    {
        // get the tails position
        vec3_t tail_position = vec3(analog_point_draw_position(end_point),
                                    position.y + track_subbeat_position(TRACK_ANALOG_SLAM_SUBBEATS),
                                    0);

        // create the tail vertices
        mesh_set_vertices_quad(mesh,
                               *vertices_index,
                               TRACK_ANALOG_WIDTH,
                               track_subbeat_position(TRACK_ANALOG_SLAM_SUBBEATS),
                               tail_position);

        // increment vertices_index
        *vertices_index += ANALOG_MESH_SLAM_TAIL_SIZE;
    }
}

void load_analogs(AnalogMesh *mesh)
{
    // the offset for the current segments vertices
    int vertices_index = 0;

    for (int l = 0; l < CHART_ANALOG_LANES; l++)
    {
        for (int a = 0; a < mesh->chart->num_analogs[l]; a++)
        {
            Analog *analog = &mesh->chart->analogs[l][a];

            for (int p = 0; p < analog->num_points - 1; p++)
            {
                AnalogPoint *start_point = &analog->points[p];
                AnalogPoint *end_point = &analog->points[p + 1];

                // create the vertices for the current segment
                if (end_point->slam)
                    create_slam_vertices(mesh->mesh,
                                         &vertices_index,
                                         start_point,
                                         end_point,
                                         p == analog->num_points - 2);
                else
                    create_segment_vertices(mesh->mesh,
                                            &vertices_index,
                                            start_point,
                                            end_point);
            }
        }
    }
}

AnalogMesh *analog_mesh_create(Chart *chart)
{
    // create the mesh
    AnalogMesh *mesh = malloc(sizeof(AnalogMesh));

    // set the meshes properties
    mesh->chart = chart;

    // create the program
    mesh->program = program_create("analog.vs", "analog.fs", true);
    mesh->uniform_speed_id = program_get_uniform_id(mesh->program, "speed");
    mesh->uniform_lane_id = program_get_uniform_id(mesh->program, "lane");

    // get the size of all the segments in the given charts analogs
    size_t mesh_size = 0;
    for (int l = 0; l < CHART_ANALOG_LANES; l++)
    {
        for (int a = 0; a < chart->num_analogs[l]; a++)
        {
            Analog *analog = &chart->analogs[l][a];

            for (int p = 0; p < analog->num_points - 1; p++)
            {
                AnalogPoint *start_point = &analog->points[p];
                AnalogPoint *end_point = &analog->points[p + 1];

                if (end_point->slam)
                {
                    mesh_size += ANALOG_MESH_SLAM_SIZE;

                    if (p == analog->num_points - 2)
                        mesh_size += ANALOG_MESH_SLAM_TAIL_SIZE;
                }
                else
                    mesh_size += ANALOG_MESH_SEGMENT_SIZE;
            }
        }
    }

    // create the analogs mesh
    mesh->mesh = mesh_create(mesh_size, mesh->program, GL_STATIC_DRAW);

    // load the analogs
    load_analogs(mesh);

    // return the mesh
    return mesh;
}

void analog_mesh_free(AnalogMesh *mesh)
{
    // free the program
    program_free(mesh->program);

    // free the analogs mesh
    mesh_free(mesh->mesh);

    // free the mesh
    free(mesh);
}

void analog_mesh_draw(AnalogMesh *mesh,
                      mat4_t projection,
                      mat4_t view,
                      mat4_t model,
                      uint16_t start_subbeat,
                      uint16_t end_subbeat,
                      double speed)
{
    // additive blending for analogs
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // raise analogs slightly above the track
    model = m4_mul(model, m4_translation(vec3(0, 0, -0.005)));

    // draw the analogs
    program_use(mesh->program);
    program_set_matrices(mesh->program, projection, view, model);
    glUniform1f(mesh->uniform_speed_id, speed);
    mesh_draw_start(mesh->mesh);

    int vertices_index = 0;
    for (int l = 0; l < CHART_ANALOG_LANES; l++)
    {
        glUniform1i(mesh->uniform_lane_id, l);

        for (int a = 0; a < mesh->chart->num_analogs[l]; a++)
        {
            Analog *analog = &mesh->chart->analogs[l][a];

            for (int p = 0; p < analog->num_points - 1; p++)
            {
                AnalogPoint *start_point = &analog->points[p];
                AnalogPoint *end_point = &analog->points[p + 1];

                // the size, in vertices, of the current segment
                int segment_size;

                // get the size of the current segment
                if (end_point->slam)
                {
                    segment_size = ANALOG_MESH_SLAM_SIZE;

                    if (p == analog->num_points - 2)
                        segment_size += ANALOG_MESH_SLAM_TAIL_SIZE;
                }
                else
                    segment_size = ANALOG_MESH_SEGMENT_SIZE;

                // draw the current segment if it is in range
                if (end_point->subbeat >= start_subbeat &&
                    start_point->subbeat <= end_subbeat)
                    mesh_draw_vertices(mesh->mesh, vertices_index, segment_size);

                // increment the vertices index
                vertices_index += segment_size;
            }
        }
    }

    mesh_draw_end(mesh->mesh);
}
