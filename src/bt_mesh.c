#include "bt_mesh.h"

#include "chart.h"
#include "track.h"

NoteMesh *bt_mesh_create()
{
    // create and return the note mesh
    return note_mesh_create("bt", CHART_BT_LANES);
}

void bt_mesh_load(NoteMesh *mesh, Chart *chart, uint16_t start_subbeat, uint16_t end_subbeat, double speed)
{
    // load the notes for the given mesh
    note_mesh_load(mesh,
                   CHART_BT_LANES,
                   chart->num_bt_notes,
                   chart->bt_notes,
                   TRACK_BT_WIDTH,
                   start_subbeat,
                   end_subbeat,
                   speed);
}
