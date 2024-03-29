#include "audio_track.h"

#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>

#include "bass_utils.h"

AudioTrack *audio_track_create(const char *path)
{
    AudioTrack *track = malloc(sizeof(AudioTrack));

    // load the track
    if (!(track->stream = BASS_StreamCreateFile(FALSE, path, 0, 0, 0)))
    {
        char message[30 + PATH_MAX];
        sprintf(message, "unable to load track \"%s\"", path);
        bass_error(message);
    }

    return track;
}

void audio_track_free(AudioTrack *track)
{
    BASS_StreamFree(track->stream);
    free(track);
}

void audio_track_play(AudioTrack *track)
{
    // restart and play the track
    BASS_ChannelPlay(track->stream, TRUE);
}

double audio_track_position(AudioTrack *track)
{
    QWORD position_bytes;
    double position;

    // get the position in bytes
    if ((position_bytes = BASS_ChannelGetPosition(track->stream, BASS_POS_BYTE)) == -1)
        bass_error("unable to get track position");

    // get the position in seconds
    if ((position = BASS_ChannelBytes2Seconds(track->stream, position_bytes)) < 0)
        bass_error("unable to convert track position bytes to seconds");

    // convert the position to milliseconds
    position *= 1000;

    return position;
}
