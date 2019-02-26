#include "scoring.h"

#include "judgement.h"

Scoring *scoring_create(Chart *chart)
{
    // create the scoring
    Scoring *scoring = malloc(sizeof(Scoring));

    // set the scorings properties
    scoring->chart = chart;

    // default the current bt and fx notes to null
    for (int i = 0; i < CHART_BT_LANES; i++)
        scoring->current_bt_notes[i] = NULL;

    for (int i = 0; i < CHART_FX_LANES; i++)
        scoring->current_fx_notes[i] = NULL;

    // allocate all the judged arrays
    for (int i = 0; i < CHART_BT_LANES; i++)
        scoring->bt_notes_judged[i] = calloc(chart->num_bt_notes[i], sizeof(bool));

    for (int i = 0; i < CHART_FX_LANES; i++)
        scoring->fx_notes_judged[i] = calloc(chart->num_fx_notes[i], sizeof(bool));

    // return the scoring
    return scoring;
}

void scoring_free(Scoring *scoring)
{
    // free all the judged arrays
    for (int i = 0; i < CHART_BT_LANES; i++)
        free(scoring->bt_notes_judged[i]);

    for (int i = 0; i < CHART_FX_LANES; i++)
        free(scoring->fx_notes_judged[i]);

    // free the scoring
    free(scoring);
}

Judgement note_passed(Note **current_notes,
                     bool **notes_judged,
                     Note **notes,
                     int lane,
                     int index)
{
    // the judgement to return
    Judgement judgement = JudgementNone;

    // if the note is a chip and was not judged
    if (!notes[lane][index].hold && !notes_judged[lane][index])
    {
        // set the judgement to error
        judgement = JudgementError;

        // mark the note as now judged
        notes_judged[lane][index] = true;
    }

    // reset the current note for the given lane
    current_notes[lane] = NULL;

    // return the judgement
    return judgement;
}

Judgement scoring_bt_note_passed(Scoring *scoring, int lane, int index)
{
    // handle the passed note and return the judgement
    return note_passed(scoring->current_bt_notes,
                       scoring->bt_notes_judged,
                       scoring->chart->bt_notes,
                       lane,
                       index);
}

Judgement scoring_fx_note_passed(Scoring *scoring, int lane, int index)
{
    // handle the passed note and return the judgement
    return note_passed(scoring->current_fx_notes,
                       scoring->fx_notes_judged,
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
                             bool **notes_judged,
                             int lane,
                             bool pressed,
                             double time)
{
    // if there is a current note on the given lane
    if (current_notes[lane])
    {
        // return if the current note is already judged
        if (notes_judged[lane][current_note_indexes[lane]])
            return JudgementNone;

        // get the current note for the given lane
        Note *note = current_notes[lane];

        // if the current note is a hold
        if (note->hold)
        {
            // todo: hold processing
        }
        // if the current note is a chip and the button is pressed
        else if (pressed)
        {
            // mark the note as judged
            notes_judged[lane][current_note_indexes[lane]] = true;

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
                              scoring->bt_notes_judged,
                              lane,
                              pressed,
                              time);
}

Judgement scoring_fx_state_changed(Scoring *scoring, int lane, bool pressed, double time)
{
    // process the given state
    return note_state_changed(scoring->current_fx_notes,
                              scoring->current_fx_note_indexes,
                              scoring->fx_notes_judged,
                              lane,
                              pressed,
                              time);
}
