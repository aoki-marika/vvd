#include "bass_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <bass/bass.h>

void bass_error(const char *message)
{
    fprintf(stderr, "bass: error(%d): %s\n", BASS_ErrorGetCode(), message);
    exit(1);
}
