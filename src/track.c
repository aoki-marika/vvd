#include "track.h"

#include <stdlib.h>

#define MATH_3D_IMPLEMENTATION
#include <arkanis/math_3d.h>

#include "screen.h"

Track *track_create(Chart *chart)
{
    // create the track
    Track *track = malloc(sizeof(Track));

    // set the track properties
    track->chart = chart;

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

    // free the track
    free(track);
}

void track_draw(Track *track, double time)
{
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
