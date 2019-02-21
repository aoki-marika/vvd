#pragma once

#include <stdint.h>

#include "chart.h"

// the maximum amount values in a data line
// todo: test with more charts
#define VOX_DATA_LINE_MAX_VALUES 8

typedef enum
{
    VoxSectionNone,          //none yet or END
    VoxSectionFormatVersion, //FORMAT VERSION
    VoxSectionEndPosition,   //END POSITION or END POSISION
    VoxSectionBeatInfo,      //BEAT INFO
    VoxSectionBpmInfo,       //BPM INFO
    VoxSectionTrackAnalogL,  //TRACK1
    VoxSectionTrackAnalogR,  //TRACK8
    VoxSectionTrackBtA,      //TRACK3
    VoxSectionTrackBtB,      //TRACK4
    VoxSectionTrackBtC,      //TRACK5
    VoxSectionTrackBtD,      //TRACK6
    VoxSectionTrackFxL,      //TRACK2
    VoxSectionTrackFxR,      //TRACK7
} VoxSection;

typedef enum
{
    VoxAnalogStateContinue = 0,
    VoxAnalogStateStart = 1,
    VoxAnalogStateEnd = 2,
} VoxAnalogState;

typedef struct
{
    // the current section the parser is in
    VoxSection section;

    // the format version of the parsing vox file
    uint8_t format_version;

    // the currently building analogs for each lane
    Analog *building_analogs[CHART_ANALOG_LANES];
} VoxParsingState;

void *chart_vox_parsing_state_create();
void chart_vox_parsing_state_free(void *parsing_state);

void chart_vox_parse_line(Chart *chart, void *parsing_state, char *line);
