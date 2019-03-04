#include "fx_mesh.h"

#include "track.h"

NoteMesh *fx_mesh_create(Chart *chart)
{
    // create and return the note mesh
    return note_mesh_create("fx",
                            CHART_FX_LANES,
                            chart->num_fx_notes,
                            chart->fx_notes,
                            FX_MESH_NOTE_WIDTH);
}
