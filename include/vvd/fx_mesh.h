#pragma once

#include <stdint.h>

#include "note_mesh.h"
#include "chart.h"

NoteMesh *fx_mesh_create();

// load note vertices from the given chart that are between start_subbeat and end_subbeat into the given mesh
void fx_mesh_load(NoteMesh *mesh, Chart *chart, uint16_t start_subbeat, uint16_t end_subbeat, double speed);

