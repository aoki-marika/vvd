#pragma once

typedef struct
{
	int device;
} Audio;

// note that this must be called before any samples or tracks are loaded and/or played
Audio *audio_create();
void audio_free(Audio *audio);
