#include "note_utils.h"

uint16_t note_time_at_beat_to_subbeat(Beat *note_beat, uint16_t measure, uint8_t beat, uint8_t subbeat)
{
    // calculate the difference in subbeats between note_beat and the given note time
    uint16_t difference = ((measure - note_beat->measure) * note_beat->numerator + beat) * (CHART_BEAT_SUBBEATS * 4 / note_beat->denominator) + subbeat;

    // return the difference offset by note_beats subbeat
    return note_beat->subbeat + difference;
}

uint16_t note_time_to_subbeat(Chart *chart, uint16_t measure, uint8_t beat, uint8_t subbeat)
{
    // get the index of the beat for the given note timing
    int beat_index = 0;

    for (int i = 0; i < chart->num_beats; i++)
    {
        // break if the current beat is after the given note time
        if (chart->beats[i].measure > measure)
            break;

        beat_index = i;
    }

    // return the subbeat
    return note_time_at_beat_to_subbeat(&chart->beats[beat_index], measure, beat, subbeat);
}

double subbeats_at_tempo_to_duration(Tempo *tempo, uint16_t subbeats)
{
    // calculate and return the duration in milliseconds of the given number of subbeats at the given tempo
    return ((double)subbeats / CHART_BEAT_SUBBEATS) * (60.0f / tempo->bpm) * 1000.0;
}
