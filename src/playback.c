#include "playback.h"

#include <stdlib.h>
#include <assert.h>

#include "scoring.h"
#include "timing.h"
#include "screen.h"
#include "note_utils.h"

Playback *playback_create(Chart *chart, AudioTrack *audio_track, Track *track, Scoring *scoring)
{
    // create the playback
    Playback *playback = malloc(sizeof(Playback));

    // set the playbacks properties
    playback->chart = chart;
    playback->audio_track = audio_track;
    playback->track = track;
    playback->scoring = scoring;
    playback->started = false;
    playback->tempo_index = 0;

    // default all the current notes/analogs to none
    for (int i = 0; i < CHART_BT_LANES; i++)
        playback->current_bt_notes[i] = PLAYBACK_CURRENT_NONE;

    for (int i = 0; i < CHART_FX_LANES; i++)
        playback->current_fx_notes[i] = PLAYBACK_CURRENT_NONE;

    for (int i = 0; i < CHART_ANALOG_LANES; i++)
    {
        playback->current_analogs[i] = PLAYBACK_CURRENT_NONE;
        playback->current_analogs_points[i] = PLAYBACK_CURRENT_NONE;
    }

    // return the playback
    return playback;
}

void playback_free(Playback *playback)
{
    free(playback);
}

void playback_start(Playback *playback, double delay)
{
    // set the playbacks start time
    playback->start_time = time_milliseconds() + delay;
}

void update_current_notes(int num_lanes,
                          int num_notes[num_lanes],
                          Note *notes[num_lanes],
                          int current_notes[num_lanes],
                          double time)
{
    for (int l = 0; l < num_lanes; l++)
    {
        // clear the current lanes current note
        current_notes[l] = PLAYBACK_CURRENT_NONE;

        for (int n = 0; n < num_notes[l]; n++)
        {
            Note *note = &notes[l][n];

            // get whether or not the current notes maximum timing window is in range of time
            bool in_range = false;

            if (note->hold)
                in_range = (time >= note->start_time - JUDGEMENT_ERROR_WINDOW) &&
                           (time <= note->end_time + JUDGEMENT_ERROR_WINDOW);
            else
                in_range = (time >= note->start_time - JUDGEMENT_ERROR_WINDOW) &&
                           (time <= note->start_time + JUDGEMENT_ERROR_WINDOW);

            // set the current lanes current note if the current note is in range
            if (in_range)
            {
                current_notes[l] = n;
                break;
            }
            // break if the current note is after time, as no notes afterwards can be in range
            else if (note->start_time + JUDGEMENT_ERROR_WINDOW > time)
            {
                break;
            }
        }
    }
}

void update_current_analogs(Playback *playback, double time)
{
    for (int l = 0; l < CHART_ANALOG_LANES; l++)
    {
        // clear the current lanes current analog and point
        playback->current_analogs[l] = PLAYBACK_CURRENT_NONE;
        playback->current_analogs_points[l] = PLAYBACK_CURRENT_NONE;

        for (int a = 0; a < playback->chart->num_analogs[l]; a++)
        {
            Analog *analog = &playback->chart->analogs[l][a];

            // whether or not this lane is finished finding a current segment
            bool lane_finished = false;

            for (int p = 0; p < analog->num_points - 1; p++)
            {
                AnalogPoint *start_point = &analog->points[p];
                AnalogPoint *end_point = &analog->points[p + 1];

                // get whether or not the current segments maximum timing window is in range of time
                bool in_range = false;

                if (end_point->slam)
                    in_range = (time >= start_point->time - JUDGEMENT_ANALOG_SLAM_WINDOW) &&
                               (time <= end_point->time + JUDGEMENT_ANALOG_SLAM_WINDOW);
                else
                    in_range = (time >= start_point->time) &&
                               (time <= end_point->time);

                // set the current lanes current analog and point if the current segment is in range
                if (in_range)
                {
                    playback->current_analogs[l] = a;
                    playback->current_analogs_points[l] = p;
                    lane_finished = true;
                    break;
                }
                // break if the current segment is after time, as no segments afterwards can be in range
                else if (start_point->time > time)
                {
                    lane_finished = true;
                    break;
                }
            }

            // break if this lane is finished finding a current segment
            if (lane_finished)
                break;
        }
    }
}

void send_current_notes_events(Scoring *scoring,
                               int num_lanes,
                               int last_notes[num_lanes],
                               int current_notes[num_lanes],
                               Judgement (* scoring_note_passed)(Scoring *, int, int),
                               void (* scoring_note_current)(Scoring *, int, int))
{
    // check for changes between the last and current notes
    for (int i = 0; i < num_lanes; i++)
    {
        // get the current and last notes
        int current = current_notes[i];
        int last = last_notes[i];

        // if the current note changed
        if (current != last)
        {
            // pass the current events
            // only pass an event if it has a note

            if (last != PLAYBACK_CURRENT_NONE)
                // todo: show the judgement on the critical line
                scoring_note_passed(scoring, i, last);

            if (current != PLAYBACK_CURRENT_NONE)
                scoring_note_current(scoring, i, current);
        }
    }
}

void update_current(Playback *playback, double time)
{
    // store the last notes to compare against the current
    int last_bt_notes[CHART_BT_LANES];
    int last_fx_notes[CHART_FX_LANES];

    for (int i = 0; i < CHART_BT_LANES; i++)
        last_bt_notes[i] = playback->current_bt_notes[i];

    for (int i = 0; i < CHART_FX_LANES; i++)
        last_fx_notes[i] = playback->current_fx_notes[i];

    // update the current bt notes
    update_current_notes(CHART_BT_LANES,
                         playback->chart->num_bt_notes,
                         playback->chart->bt_notes,
                         playback->current_bt_notes,
                         time);

    // update the current fx notes
    update_current_notes(CHART_FX_LANES,
                         playback->chart->num_fx_notes,
                         playback->chart->fx_notes,
                         playback->current_fx_notes,
                         time);

    // update the current analogs
    update_current_analogs(playback, time);

    // send the current bt notes events
    send_current_notes_events(playback->scoring,
                              CHART_BT_LANES,
                              last_bt_notes,
                              playback->current_bt_notes,
                              scoring_bt_note_passed,
                              scoring_bt_note_current);

    // send the current fx notes events
    send_current_notes_events(playback->scoring,
                              CHART_FX_LANES,
                              last_fx_notes,
                              playback->current_fx_notes,
                              scoring_fx_note_passed,
                              scoring_fx_note_current);
}

void playback_tick(Playback *playback, double subbeat)
{
    // get the tick size for the current tempo
    int tick_size = PLAYBACK_TICK_SIZE;
    if (playback->chart->tempos[playback->tempo_index].bpm >= PLAYBACK_HALF_TICK_RATE_BPM)
        tick_size *= 2;

    // get the current tick
    int tick = floor(subbeat / tick_size);

    // if the current tick has changed since the last update
    if (tick != playback->last_tick)
    {
        Judgement bt_hold_judgements[CHART_BT_LANES];
        Judgement fx_hold_judgements[CHART_FX_LANES];

        // tick the given playbacks scoring
        scoring_tick_changed(playback->scoring,
                             tick,
                             tick * tick_size,
                             bt_hold_judgements,
                             fx_hold_judgements);

        // todo: process tick judgements
    }

    // set the given playbacks last tick
    playback->last_tick = tick;
}

bool playback_update(Playback *playback)
{
    // get the current time, relative to start_time
    double relative_time = time_milliseconds() - playback->start_time;

    // if playback has not yet started and time is past the start time
    if (!playback->started && relative_time >= 0)
    {
        // start the audio
        audio_track_play(playback->audio_track);

        // mark the playback as started
        playback->started = true;
    }

    // say playback is finished if the current time is after the charts end time
    if (relative_time >= playback->chart->end_time)
        return true;

    // update the given playbacks tempo index
    for (int i = 0; i < playback->chart->num_tempos; i++)
    {
        // break if the current tempo is after relative time
        if (playback->chart->tempos[i].time > relative_time)
            break;

        // set the given playbacks tempo index
        playback->tempo_index = i;
    }

    // update the current notes/analogs
    update_current(playback, relative_time);

    // get relative time in subbeats
    double relative_time_subbeat = time_to_subbeat(playback->chart, playback->tempo_index, relative_time);

    // tick the given playback
    playback_tick(playback, relative_time_subbeat);

    // draw the track
    // draw at subbeat 0 if playback has not started yet so theres no scroll in before starting
    track_draw(playback->track, playback->tempo_index, (!playback->started) ? 0 : relative_time_subbeat);

    // say playback is not finished
    return false;
}

void playback_note_state_changed(Playback *playback,
                                 int num_lanes,
                                 Note *notes[num_lanes],
                                 int current_notes[num_lanes],
                                 int lane,
                                 bool pressed,
                                 Judgement (* scoring_state_changed)(Scoring *scoring, int, bool, double),
                                 void (* track_beam)(Track *, int, Judgement))
{
    // assert that lane is valid
    assert(lane >= 0 && lane < num_lanes);

    // get the current time relative to the given playbacks starts time
    double relative_time = time_milliseconds() - playback->start_time;

    // pass the event to the given method
    Judgement judgement = scoring_state_changed(playback->scoring, lane, pressed, relative_time);

    // show the beam for the judgement if its not none
    if (judgement != JudgementNone)
        track_beam(playback->track, lane, judgement);
    // show an error beam if the given state is pressed and judgement was none, and theres no current note or hold
    // this is to replicate sdvx in showing beams when pressing buttons without notes
    else if (pressed &&
             judgement == JudgementNone &&
             (current_notes[lane] == PLAYBACK_CURRENT_NONE || !notes[lane][current_notes[lane]].hold))
        track_beam(playback->track, lane, JudgementError);
    // todo: if the state is pressed and the current note is a hold, this is where hold active/inactive state should be set
}

void playback_bt_state_changed(Playback *playback, int lane, bool pressed)
{
    // process the given event
    playback_note_state_changed(playback,
                                CHART_BT_LANES,
                                playback->chart->bt_notes,
                                playback->current_bt_notes,
                                lane,
                                pressed,
                                scoring_bt_state_changed,
                                track_bt_beam);
}

void playback_fx_state_changed(Playback *playback, int lane, bool pressed)
{
    // process the given event
    playback_note_state_changed(playback,
                                CHART_FX_LANES,
                                playback->chart->fx_notes,
                                playback->current_fx_notes,
                                lane,
                                pressed,
                                scoring_fx_state_changed,
                                track_fx_beam);
}
