#include "note_utils.h"

#include <assert.h>

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

double time_to_subbeat(Chart *chart, int tempo_index, double time)
{
    // assert that tempo_index is valid
    assert(tempo_index >= 0 && tempo_index < chart->num_tempos);

    // get the given tempo
    Tempo *tempo = &chart->tempos[tempo_index];

    // get the start time and subbeat of tempo
    double start_time = tempo->time;
    double start_subbeat = tempo->subbeat;

    // get the end time and subbeat of tempo
    double end_time;
    double end_subbeat;

    // get the next tempos time and subbeat if there is a next tempo
    if (tempo_index + 1 < chart->num_tempos)
    {
        Tempo *next_tempo = &chart->tempos[tempo_index + 1];
        end_time = next_tempo->time;
        end_subbeat = next_tempo->subbeat;
    }
    // get the charts end time and subbeat if there is no next tempo
    else
    {
        double duration = subbeats_at_tempo_to_duration(tempo, chart->end_subbeat - tempo->subbeat);
        end_time = start_time + duration;
        end_subbeat = chart->end_subbeat;
    }

    // calculate the percentage between start_time and end_time time is at
    float time_percentage = (time - start_time) / (end_time - start_time);

    // calculate and return the subbeat of time by multiplying the difference between start_subbeat and end_subbeat and offsetting it by start_subbeat
    return start_subbeat + ((end_subbeat - start_subbeat)  * time_percentage);
}
