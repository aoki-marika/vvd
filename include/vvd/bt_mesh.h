#pragma once

#include "note_mesh.h"
#include "chart.h"

// the draw width of a bt note
#define BT_MESH_NOTE_WIDTH (TRACK_NOTES_WIDTH / CHART_BT_LANES)

// create a note mesh for the fx notes in the given chart
NoteMesh *bt_mesh_create(Chart *chart);
