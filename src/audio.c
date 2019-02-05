#include "audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#define SAMPLE_RATE 44100

void assert_pa_error(PaError error)
{
    if (error != paNoError)
    {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(error));
        exit(1);
    }
}

void assert_type(int type)
{
    assert(type == AUDIO_SAMPLE || type == AUDIO_TRACK);
}

int portaudio_callback(const void *input_buffer,
                       void *output_buffer,
                       unsigned long frame_count,
                       const PaStreamCallbackTimeInfo *time_info,
                       PaStreamCallbackFlags status_flags,
                       void *user_data)
{
    // todo: playing numerous files at once gets glitchy sounding and can cut off and/or repeat a frame until the next file is played
    // this is most likely due to the performance of the callback as it gets worse the more files are being played
    // but also not 100% sure as the track audio seems to play fine while samples are broken?
    // getting quite annoying so leaving it for now

    Audio *audio = (Audio *)user_data;
    float *output = (float *)output_buffer;

    // iterate each currently playing audio file
    for (int i = 0; i < audio->num_playing; i++)
    {
        AudioPlaying *audio_playing = &audio->playing[i];

        // skip finished files
        if (audio->playing[i].finished)
            continue;

        // read the audio data
        uint64_t data_size = frame_count * audio_playing->file->info.channels;
        float data[data_size];

        uint64_t data_length = audio_file_read(audio_playing->file, audio_playing->position, data_size, data);

        // increment the playing position
        audio_playing->position += data_length;

        // if the read data length is less than a frame of data then reading is at eof
        audio_playing->finished = data_length < data_size;

        // merge the audio data into the output buffer
        for (int oi = 0; oi < data_size; oi++)
        {
            float value = data[oi];

            // the first value set in the buffer needs to be set as the default value can be anything
            if (i == 0)
                output[oi] = value;
            else
                output[oi] += value;
        }
    }

    // the stream is never stopped from here as audio can be added/removed at any time
    return paContinue;
}

Audio *audio_get()
{
    // initialize portaudio
    assert_pa_error(Pa_Initialize());

    // create the audio
    Audio *audio = malloc(sizeof(Audio));

    // set the audios default values
    audio->current_id = 0;
    audio->num_samples = 0;
    audio->num_tracks = 0;
    audio->num_playing = 0;

    // open the output stream
    assert_pa_error(Pa_OpenDefaultStream(&audio->stream,
                                         0, //no input
                                         2, //stereo output
                                         paFloat32, //32 bit float output
                                         SAMPLE_RATE,
                                         paFramesPerBufferUnspecified,
                                         portaudio_callback,
                                         audio));

    // start the output stream
    assert_pa_error(Pa_StartStream(audio->stream));

    // return an allocated audio
    return audio;
}

void audio_free(Audio *audio)
{
    // close the stream
    assert_pa_error(Pa_CloseStream(audio->stream));

    // free all the samples
    for (int i = 0; i < audio->num_samples; i++)
        audio_file_free(audio->samples[i]);

    // free all the tracks
    for (int i = 0; i < audio->num_tracks; i++)
        audio_file_free(audio->tracks[i]);

    // free the audio
    free(audio);

    // terminate portaudio
    assert_pa_error(Pa_Terminate());
}

void audio_update(Audio *audio)
{
    // remove all the finished audio files
    for (int i = 0; i < audio->num_playing - 1; i++)
    {
        if (!audio->playing[i].finished)
            continue;

        // remove the finished playing audio from the playing array by shifting everything after one back
        audio->playing[i] = audio->playing[i + 1];

        // update num_playing
        audio->num_playing--;
    }
}

int audio_add(Audio *audio, int type, const char *path)
{
    assert_type(type);

    // open the audio file
    AudioFile *audio_file = audio_file_open(path, type == AUDIO_SAMPLE);

    // set the audio files id and increment the audios current id
    audio_file->id = audio->current_id;
    audio->current_id++;

    // add the audio file to the respective array
    switch (type)
    {
        case AUDIO_SAMPLE:
            audio->samples[audio->num_samples] = audio_file;
            audio->num_samples++;
            break;
        case AUDIO_TRACK:
            audio->tracks[audio->num_tracks] = audio_file;
            audio->num_tracks++;
            break;
    }

    // return the audio files id
    return audio_file->id;
}

void audio_play(Audio *audio, int type, int id)
{
    assert_type(type);

    // the audio file to play
    AudioFile *audio_file;

    // find the audio file
    switch (type)
    {
        case AUDIO_SAMPLE:
            for (int i = 0; i < audio->num_samples; i++)
                if (audio->samples[i]->id == id)
                    audio_file = audio->samples[i]; break;
            break;
        case AUDIO_TRACK:
            for (int i = 0; i < audio->num_tracks; i++)
                if (audio->tracks[i]->id == id)
                    audio_file = audio->tracks[i]; break;
            break;
    }

    // assert that an audio file was found
    assert(audio_file);

    // add the audio file to the playing array
    AudioPlaying playing = (AudioPlaying)
    {
        .position = 0,
        .finished = false,
        .file = audio_file,
    };

    audio->playing[audio->num_playing] = playing;
    audio->num_playing++;
}
