#include "bt_mesh.h"

#include "track.h"

NoteMesh *bt_mesh_create(Chart *chart)
{
    // create and return the note mesh
    return note_mesh_create("bt",
                            CHART_BT_LANES,
                            chart->num_bt_notes,
                            chart->bt_notes,
                            TRACK_BT_WIDTH);
}
