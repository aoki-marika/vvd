#include "audio_sample.h"

#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>

#include "bass_utils.h"

AudioSample *audio_sample_create(const char *path)
{
    AudioSample *sample = malloc(sizeof(AudioSample));

    // load the sample
    if (!(sample->sample = BASS_SampleLoad(FALSE, path, 0, 0, 10, 0)))
    {
        char message[30 + PATH_MAX];
        sprintf(message, "unable to load sample \"%s\"", path);
        bass_error(message);
    }

    return sample;
}

void audio_sample_free(AudioSample *sample)
{
    BASS_SampleFree(sample->sample);
    free(sample);
}

void audio_sample_play(AudioSample *sample)
{
    // get a channel for the sample
    HCHANNEL channel = BASS_SampleGetChannel(sample->sample, FALSE);

    // play the channel
    BASS_ChannelPlay(channel, TRUE);
}
