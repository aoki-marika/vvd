#pragma once

#include "note_mesh.h"
#include "chart.h"

// the draw width of an fx note
#define FX_MESH_NOTE_WIDTH (TRACK_NOTES_WIDTH / CHART_FX_LANES)

// create a note mesh for the fx notes in the given chart
NoteMesh *fx_mesh_create(Chart *chart);
