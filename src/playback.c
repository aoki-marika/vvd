#include "playback.h"

#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>

#include "scoring.h"

double current_milliseconds()
{
    // get the current time of day
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // convert tv_sec and tv_usec to ms and return
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

Playback *playback_create(Chart *chart, AudioTrack *audio_track, Track *track)
{
    // create the playback
    Playback *playback = malloc(sizeof(Playback));

    // set the playbacks properties
    playback->chart = chart;
    playback->audio_track = audio_track;
    playback->track = track;
    playback->started = false;

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
    playback->start_time = current_milliseconds() + delay;
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
                in_range = (time >= note->start_time) &&
                           (time <= note->end_time);
            else
                in_range = (time >= note->start_time - SCORING_ERROR_WINDOW) &&
                           (time <= note->start_time + SCORING_ERROR_WINDOW);

            // set the current lanes current note if the current note is in range
            if (in_range)
            {
                current_notes[l] = n;
                break;
            }
            // break if the current note is after time, as no notes afterwards can be in range
            else if (note->start_time + SCORING_ERROR_WINDOW > time)
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
                    in_range = (time >= start_point->time - SCORING_ANALOG_SLAM_WINDOW) &&
                               (time <= end_point->time + SCORING_ANALOG_SLAM_WINDOW);
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

void update_current(Playback *playback, double time)
{
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
}

bool playback_update(Playback *playback)
{
    // get the current time, relative to start_time
    double relative_time = current_milliseconds() - playback->start_time;

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

    // update the current notes/analogs
    update_current(playback, relative_time);

    // draw the track
    // draw at time 0 if playback has not started yet so theres no scroll in before starting
    track_draw(playback->track, (!playback->started) ? 0 : relative_time);

    // say playback is not finished
    return false;
}
