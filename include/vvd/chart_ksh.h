#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "chart.h"

// max values
#define KSH_MEASURE_LINES_MAX 256

typedef struct
{
    // whether or not parsing is currently in the metadata section
    bool metadata;

    // the current measure being parsed
    int measure;

    // the count of the total lines and note lines of the current measure, respectively
    int num_measure_lines;
    int num_measure_notes;

    // the current measures lines (notes and k/v pairs)
    char *measure_lines[KSH_MEASURE_LINES_MAX];

    // the currently processing bt holds
    Note *processing_bt_holds[CHART_BT_LANES];

    // the currently processing fx holds
    Note *processing_fx_holds[CHART_FX_LANES];

    // thek last note that was added
    Note *last_note;
} KSHParsingState; //todo: KSH -> Ksh

void *chart_ksh_parsing_state_create();
void chart_ksh_parsing_state_free(void *parsing_state);

void chart_ksh_parse_line(Chart *chart, void *parsing_state, char *line);
