#pragma once

#include "chart.h"
#include "track.h"

typedef struct
{
    // the chart for this scoring to pull notes and analogs from
    Chart *chart;

    // the current and index of the current bt notes for each lane
    Note *current_bt_notes[CHART_BT_LANES];
    int current_bt_note_indexes[CHART_BT_LANES];

    // the current and index of the current fx notes for each lane
    Note *current_fx_notes[CHART_FX_LANES];
    int current_fx_note_indexes[CHART_FX_LANES];

    // whether or not each bt/fx chip in chart has been judged
    bool *bt_chips_judged[CHART_BT_LANES];
    bool *fx_chips_judged[CHART_FX_LANES];

    // whether or not the button of each lane is held on the current hold, if any
    bool bt_holds_held[CHART_BT_LANES];
    bool fx_holds_held[CHART_FX_LANES];
} Scoring;

Scoring *scoring_create(Chart *chart);
void scoring_free(Scoring *scoring);

// tell the given scoring that the given bt/fx note in the given lane has become current
void scoring_bt_note_current(Scoring *scoring, int lane, int index);
void scoring_fx_note_current(Scoring *scoring, int lane, int index);

// tell the given scoring that the bt/fx button for the given lane has changed states to the given state, at the given time
// time should be relative to the beginning of the given scorings chart
// returns the judgement, if any, for the given state on the given lane at the given time
// chips return the judgement for the closest matching timing window for the given time if the given state is pressed
Judgement scoring_bt_state_changed(Scoring *scoring, int lane, bool pressed, double time);
Judgement scoring_fx_state_changed(Scoring *scoring, int lane, bool pressed, double time);

// tell the given scoring that the current tick changed to the given tick, which occured at the given subbeat
// sets bt_hold_judgements and fx_hold_judgements to the judgement for the given tick of each hold on each lane, if any, of their respective types
void scoring_tick_changed(Scoring *scoring,
                          int tick,
                          int subbeat,
                          Judgement bt_hold_judgements[CHART_BT_LANES],
                          Judgement fx_hold_judgements[CHART_FX_LANES]);
