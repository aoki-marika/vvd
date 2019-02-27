#include "scoring.h"

#include "judgement.h"

Scoring *scoring_create(Chart *chart)
{
    // create the scoring
    Scoring *scoring = malloc(sizeof(Scoring));

    // set the scorings properties
    scoring->chart = chart;

    // default and allocate all the properties
    for (int i = 0; i < CHART_BT_LANES; i++)
    {
        scoring->current_bt_notes[i] = NULL;
        scoring->bt_chips_judged[i] = calloc(chart->num_bt_notes[i], sizeof(bool));
        scoring->bt_holds_held[i] = false;
    }

    for (int i = 0; i < CHART_FX_LANES; i++)
    {
        scoring->current_fx_notes[i] = NULL;
        scoring->fx_chips_judged[i] = calloc(chart->num_fx_notes[i], sizeof(bool));
        scoring->fx_holds_held[i] = false;
    }

    // return the scoring
    return scoring;
}

void scoring_free(Scoring *scoring)
{
    // free all the judged arrays
    for (int i = 0; i < CHART_BT_LANES; i++)
        free(scoring->bt_chips_judged[i]);

    for (int i = 0; i < CHART_FX_LANES; i++)
        free(scoring->fx_chips_judged[i]);

    // free the scoring
    free(scoring);
}

Judgement note_passed(Note **current_notes,
                      bool *holds_held,
                      bool **chips_judged,
                      Note **notes,
                      int lane,
                      int index)
{
    // the judgement to return
    Judgement judgement = JudgementNone;

    // if the note is a chip and was not judged
    if (!notes[lane][index].hold && !chips_judged[lane][index])
    {
        // set the judgement to error
        judgement = JudgementError;

        // mark the chip as now judged
        chips_judged[lane][index] = true;
    }

    // reset the current note and held state for the given lane
    current_notes[lane] = NULL;
    holds_held[lane] = false;

    // return the judgement
    return judgement;
}

Judgement scoring_bt_note_passed(Scoring *scoring, int lane, int index)
{
    // handle the passed note and return the judgement
    return note_passed(scoring->current_bt_notes,
                       scoring->bt_holds_held,
                       scoring->bt_chips_judged,
                       scoring->chart->bt_notes,
                       lane,
                       index);
}

Judgement scoring_fx_note_passed(Scoring *scoring, int lane, int index)
{
    // handle the passed note and return the judgement
    return note_passed(scoring->current_fx_notes,
                       scoring->fx_holds_held,
                       scoring->fx_chips_judged,
                       scoring->chart->fx_notes,
                       lane,
                       index);
}

void scoring_bt_note_current(Scoring *scoring, int lane, int index)
{
    // get the note for the given lane and index
    Note *note = &scoring->chart->bt_notes[lane][index];

    // set the given scorings current bt note and index
    scoring->current_bt_notes[lane] = note;
    scoring->current_bt_note_indexes[lane] = index;
}

void scoring_fx_note_current(Scoring *scoring, int lane, int index)
{
    // get the note for the given lane and index
    Note *note = &scoring->chart->fx_notes[lane][index];

    // set the given scorings current fx note and index
    scoring->current_fx_notes[lane] = note;
    scoring->current_fx_note_indexes[lane] = index;
}

Judgement judgement_for_chip(Note *note, double time)
{
    // critical window
    if (time >= note->start_time - JUDGEMENT_CRITICAL_WINDOW &&
        time <= note->start_time + JUDGEMENT_CRITICAL_WINDOW)
        return JudgementCritical;
    // near window
    else if (time >= note->start_time - JUDGEMENT_NEAR_WINDOW &&
             time <= note->start_time + JUDGEMENT_NEAR_WINDOW)
        return JudgementNear;
    // all other times are errors
    else
        return JudgementError;
}

Judgement note_state_changed(Note **current_notes,
                             int *current_note_indexes,
                             bool *holds_held,
                             bool **chips_judged,
                             int lane,
                             bool pressed,
                             double time)
{
    // if there is a current note on the given lane
    if (current_notes[lane])
    {
        // get the current note for the given lane
        Note *note = current_notes[lane];

        // return if the current chip is already judged
        if (!note->hold && chips_judged[lane][current_note_indexes[lane]])
            return JudgementNone;

        // if the current note is a hold
        if (note->hold)
        {
            // set the given lanes hold held state
            holds_held[lane] = pressed;
        }
        // if the current note is a chip and the button is pressed
        else if (pressed)
        {
            // mark the chip as judged
            chips_judged[lane][current_note_indexes[lane]] = true;

            // return the judgement for the current chip and time
            return judgement_for_chip(note, time);
        }
    }

    // default to returning JudgementNone
    return JudgementNone;
}

Judgement scoring_bt_state_changed(Scoring *scoring, int lane, bool pressed, double time)
{
    // process the given state
    return note_state_changed(scoring->current_bt_notes,
                              scoring->current_bt_note_indexes,
                              scoring->bt_holds_held,
                              scoring->bt_chips_judged,
                              lane,
                              pressed,
                              time);
}

Judgement scoring_fx_state_changed(Scoring *scoring, int lane, bool pressed, double time)
{
    // process the given state
    return note_state_changed(scoring->current_fx_notes,
                              scoring->current_fx_note_indexes,
                              scoring->fx_holds_held,
                              scoring->fx_chips_judged,
                              lane,
                              pressed,
                              time);
}

void tick_holds(int num_lanes,
                Note *current_notes[num_lanes],
                bool holds_held[num_lanes],
                int tick,
                int subbeat,
                Judgement judgements[num_lanes])
{
    for (int i = 0; i < num_lanes; i++)
    {
        // reset the current lanes judgement
        judgements[i] = JudgementNone;

        // if there is a current note on the current lane
        if (current_notes[i])
        {
            Note *note = current_notes[i];

			// skip if the current note is a chip
			if (!note->hold)
				continue;

            // todo: the tick count for the ending double fx holds of speedstar is 1 extra (7 instead of 6)
            // cant tell if vvd is ticking faster or if sdvx has a bug/"feature" that ending holds skip their second last tick
            // looking at videos of it it definitley looks like its just skipping the second last tick
            // all the information for ticks line up with these fx having 7 ticks, and the tick count for every other hold appears to be correct

            // the first and last ticks of holds do not produce judgements
            if (subbeat <= note->start_subbeat)
                continue;
            else if (subbeat >= note->end_subbeat)
                continue;

            // critical if the hold is being held
            if (holds_held[i])
                judgements[i] = JudgementCritical;
            // error if the hold is not being held
            else
                judgements[i] = JudgementError;
        }
    }
}

void scoring_tick_changed(Scoring *scoring,
                          int tick,
                          int subbeat,
                          Judgement bt_hold_judgements[CHART_BT_LANES],
                          Judgement fx_hold_judgements[CHART_FX_LANES])
{
    // tick bt holds
    tick_holds(CHART_BT_LANES,
               scoring->current_bt_notes,
               scoring->bt_holds_held,
               tick,
               subbeat,
               bt_hold_judgements);

    // tick fx holds
    tick_holds(CHART_FX_LANES,
               scoring->current_fx_notes,
               scoring->fx_holds_held,
               tick,
               subbeat,
               fx_hold_judgements);
}
