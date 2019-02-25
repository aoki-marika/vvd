#include "playback.h"

#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>

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

    // draw the track
    // draw at time 0 if playback has not started yet so theres no scroll in before starting
    track_draw(playback->track, (!playback->started) ? 0 : relative_time);

    // say playback is not finished
    return false;
}
