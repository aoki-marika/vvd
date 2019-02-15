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
