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

    // whether or not each bt/fx note in chart has been judged
    bool *bt_notes_judged[CHART_BT_LANES];
    bool *fx_notes_judged[CHART_FX_LANES];
} Scoring;

Scoring *scoring_create(Chart *chart);
void scoring_free(Scoring *scoring);

// Judgement scoring_[bt/fx]_note_passed(Scoring *scoring, int lane, int index)
//  - can have a JudgementError in the case of an unjudged note passing its maximum window
//
// void scoring_[bt/fx]_note_current(Scoring *scoring, int lane, int index)
//  - no judgements possible as it just sets state
//
// Judgement scoring_[bt/fx]_state_changed(Scoring *scoring, int lane, bool pressed, double time)
//  - can have JudgementCritical, JudgementNear, or JudgementError for chips
//  - should trigger beams for the given lane when judgement is crit/near/error
//
// scoring_subbeat_changed(Scoring *scoring, uint16_t subbeat, Judgement bt_holds[CHART_BT_LANES], Judgement fx_holds[CHART_FX_LANES])
//  - can have JudgementCritical or JudgementError for every hold and analog
//  - JudgementCritical means the note/analog was held and should add 1 to the chain, and set the current notes state to critical
//  - JudgementError means the note/analog was not held and should reset the chain, and set the current note/analogs state to error

// tell the given scoring that the given bt/fx note in the given lane has passed its maximum hit window after being current
// should always be called before scoring_[bt/fx]_note_current
// returns the judgement, if any, for the passed note on the given lane at the given index
// chips return a JudgementError if it was not judged before passing its maximum timing window
Judgement scoring_bt_note_passed(Scoring *scoring, int lane, int index);
Judgement scoring_fx_note_passed(Scoring *scoring, int lane, int index);

// tell the given scoring that the given bt/fx note in the given lane has become current
void scoring_bt_note_current(Scoring *scoring, int lane, int index);
void scoring_fx_note_current(Scoring *scoring, int lane, int index);

// tell the given scoring that the bt/fx button for the given lane has changed states to the given state, at the given time
// time should be relative to the beginning of the given scorings chart
// returns the judgement, if any, for the given state on the given lane at the given time
// chips return the judgement for the closest matching timing window for the given time if the given state is pressed
Judgement scoring_bt_state_changed(Scoring *scoring, int lane, bool pressed, double time);
Judgement scoring_fx_state_changed(Scoring *scoring, int lane, bool pressed, double time);
