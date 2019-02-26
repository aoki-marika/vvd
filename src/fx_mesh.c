#include "fx_mesh.h"

#include "chart.h"
#include "track.h"

NoteMesh *fx_mesh_create()
{
    // create and return the note mesh
    return note_mesh_create("fx", CHART_FX_LANES);
}

void fx_mesh_load(NoteMesh *mesh, Chart *chart, uint16_t start_subbeat, uint16_t end_subbeat, double speed)
{
    // load the notes for the given mesh
    note_mesh_load(mesh,
                   CHART_FX_LANES,
                   chart->num_fx_notes,
                   chart->fx_notes,
                   TRACK_FX_WIDTH,
                   start_subbeat,
                   end_subbeat,
                   speed);
}

