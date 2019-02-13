#include "chart_vox.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void *chart_vox_parsing_state_create()
{
    // create the parsing state
    VoxParsingState *state = malloc(sizeof(VoxParsingState));

    // set the default section to none
    state->section = VoxSectionNone;

    // return the state
    return state;
}

void chart_vox_parsing_state_free(void *parsing_state)
{
    // get the parsing state
    VoxParsingState *state = (VoxParsingState *)parsing_state;

    // free the parsing state
    free(state);
}

bool has_prefix(const char *prefix, const char *string)
{
    // return whether or not the given string has the given prefix
    return strncmp(prefix, string, strlen(prefix)) == 0;
}

uint8_t data_line_values(char *line, char *values[VOX_DATA_LINE_MAX_VALUES])
{
    // data lines in vox have values separated by tab characters

    // get the first value
    char *last_value = strtok(line, "\t");
    uint8_t num_values = 0;

    while (last_value)
    {
        // increment the number of values
        num_values++;

        // append the current value to values
        values[num_values - 1] = last_value;

        // get the next value
        last_value = strtok(NULL, "\t");
    }

    // return the number of values
    return num_values;
}

bool is_track_section(const char *section_name, uint8_t number)
{
    // track sections can be named either "TRACK[n]" or "TRACK[n] START"
    // these values are always the same size
    char track[7];
    char track_start[13];

    // format the track strings and set the respective values
    sprintf(track, "TRACK%i", number);
    sprintf(track_start, "%s START", track);

    // return whether the section name matches one of the track section formats
    return strcmp(section_name, track) == 0 ||
           strcmp(section_name, track_start) == 0;
}

void parse_section(VoxParsingState *state, char *name)
{
    // format version
    if (strcmp(name, "FORMAT VERSION") == 0)
        state->section = VoxSectionFormatVersion;
    // end position
    else if (strcmp(name, "END POSITION") == 0)
        state->section = VoxSectionEndPosition;
    // beat info
    else if (strcmp(name, "BEAT INFO") == 0)
        state->section = VoxSectionBeatInfo;
    // bpm info
    else if (strcmp(name, "BPM INFO") == 0)
        state->section = VoxSectionBpmInfo;
    // analog l
    else if (is_track_section(name, 1))
        state->section = VoxSectionTrackAnalogL;
    // analog r
    else if (is_track_section(name, 8))
        state->section = VoxSectionTrackAnalogR;
    // bt a
    else if (is_track_section(name, 3))
        state->section = VoxSectionTrackBtA;
    // bt b
    else if (is_track_section(name, 4))
        state->section = VoxSectionTrackBtB;
    // bt c
    else if (is_track_section(name, 5))
        state->section = VoxSectionTrackBtC;
    // bt d
    else if (is_track_section(name, 6))
        state->section = VoxSectionTrackBtD;
    // fx l
    else if (is_track_section(name, 2))
        state->section = VoxSectionTrackFxL;
    // fx r
    else if (is_track_section(name, 7))
        state->section = VoxSectionTrackFxR;
    // end section
    else if (strcmp(name, "END") == 0)
        state->section = VoxSectionNone;
    // print out unhandled sections for debugging
    else
        printf("parse_section: unhandled section '%s'\n", name);
}

void parse_timing(char *value, uint16_t *measure, uint8_t *beat, uint8_t *subbeat)
{
    // measure and beat are numbered starting at 1 in vox, but 0 in vvd
    *measure = atoi(strtok(value, ",")) - 1;
    *beat = atoi(strtok(NULL, ",")) - 1;
    *subbeat = atoi(strtok(NULL, ","));
}

void parse_note(Chart *chart,
                int *num_notes,
                Note *notes,
                uint8_t length,
                uint16_t measure,
                uint8_t beat,
                uint8_t subbeat)
{
    // create the note
    Note note = (Note)
    {
        .start_measure = measure,
        .start_beat = beat,
        .start_subbeat = subbeat,
    };

    // set the hold values if this note is a hold note
    if (length > 0)
    {
        // get the current number of beats per measure
        uint8_t num_measure_beats = chart->beats[chart->num_beats - 1].numerator;

        // calculate length in measures, beats, and subbeats
        uint8_t length_beats = length / CHART_BEAT_MAX_SUBBEATS;
        uint16_t length_measures = length_beats / num_measure_beats;
        uint8_t length_subbeats = length;

        length_beats -= length_measures * num_measure_beats;
        length_subbeats -= length_measures * num_measure_beats * CHART_BEAT_MAX_SUBBEATS;
        length_subbeats -= length_beats * CHART_BEAT_MAX_SUBBEATS;

        // calculate and set the end timing properties
        note.hold = true;
        note.end_measure = note.start_measure + length_measures;
        note.end_beat = note.start_beat + length_beats;
        note.end_subbeat = note.start_subbeat + length_subbeats;
    }

    // append the note to the charts notes
    notes[*num_notes] = note;
    *num_notes += 1;
}

void parse_data_line(Chart *chart, VoxParsingState *state, char *line)
{
    // get the values for the line, as most sections use them
    char *values[VOX_DATA_LINE_MAX_VALUES];
    uint8_t num_values = data_line_values(line, values);

    if (state->section == VoxSectionFormatVersion)
    {
        // format versions are just a single integer
        state->format_version = atoi(line);
    }
    else if (state->section != VoxSectionNone)
    {
        // all other section lines start with timing
        uint16_t measure;
        uint8_t beat;
        uint8_t subbeat;

        // parse the timing for the current line
        parse_timing(values[0], &measure, &beat, &subbeat);

        switch (state->section)
        {
            case VoxSectionEndPosition:
            {
                // timing
                assert(num_values == 1);

                // set the charts end timing
                chart->end_measure = measure;
                chart->end_beat = beat;
                chart->end_subbeat = subbeat;

                break;
            }
            case VoxSectionBeatInfo:
            {
                // timing, numerator, denominator
                assert(num_values == 3);

                // create the beat
                Beat chart_beat = (Beat)
                {
                    .numerator = atoi(values[1]),
                    .denominator = atoi(values[2]),
                    .measure = measure,
                    .beat = beat,
                    .subbeat = subbeat,
                };

                // append the beat to the charts beats
                chart->beats[chart->num_beats] = chart_beat;
                chart->num_beats++;

                break;
            }
            case VoxSectionBpmInfo:
            {
                // timing, bpm, ?
                // todo: is the last value important?
                // todo: booth vox dont have timing for bpm
                assert(num_values == 3);

                // create the tempo
                Tempo tempo = (Tempo)
                {
                    .bpm = atof(values[1]),
                    .measure = measure,
                    .beat = beat,
                    .subbeat = subbeat,
                };

                // append the tempo to the charts tempos
                chart->tempos[chart->num_tempos] = tempo;
                chart->num_tempos++;

                break;
            }
            case VoxSectionTrackAnalogL:
            case VoxSectionTrackAnalogR:
            {
                // todo: analog
                break;
            }
            case VoxSectionTrackBtA:
            case VoxSectionTrackBtB:
            case VoxSectionTrackBtC:
            case VoxSectionTrackBtD:
            case VoxSectionTrackFxL:
            case VoxSectionTrackFxR:
            {
                // timing, length (in subbeats), effect? (would be booth effects if anything)
                assert(num_values == 3);

                // get the respective notes and num_notes values
                int *num_notes;
                Note *notes;

                switch (state->section)
                {
                    case VoxSectionTrackBtA:
                        num_notes = &chart->num_bt_notes[CHART_BT_LANE_A];
                        notes = chart->bt_notes[CHART_BT_LANE_A];
                        break;
                    case VoxSectionTrackBtB:
                        num_notes = &chart->num_bt_notes[CHART_BT_LANE_B];
                        notes = chart->bt_notes[CHART_BT_LANE_B];
                        break;
                    case VoxSectionTrackBtC:
                        num_notes = &chart->num_bt_notes[CHART_BT_LANE_C];
                        notes = chart->bt_notes[CHART_BT_LANE_C];
                        break;
                    case VoxSectionTrackBtD:
                        num_notes = &chart->num_bt_notes[CHART_BT_LANE_D];
                        notes = chart->bt_notes[CHART_BT_LANE_D];
                        break;
                    case VoxSectionTrackFxL:
                        num_notes = &chart->num_fx_notes[CHART_FX_LANE_L];
                        notes = chart->fx_notes[CHART_FX_LANE_L];
                        break;
                    case VoxSectionTrackFxR:
                        num_notes = &chart->num_fx_notes[CHART_FX_LANE_R];
                        notes = chart->fx_notes[CHART_FX_LANE_R];
                        break;
                }

                // parse the note
                parse_note(chart,
                           num_notes,
                           notes,
                           atoi(values[1]),
                           measure,
                           beat,
                           subbeat);

                break;
            }
        }
    }
}

void chart_vox_parse_line(Chart *chart, void *parsing_state, char *line)
{
    VoxParsingState *state = (VoxParsingState *)parsing_state;

    // ignore comment and blank lines
    if (has_prefix("//", line) || strcmp(line, "") == 0)
        return;

    if (has_prefix("#", line))
    {
        // parse the section name, which is everything after the #
        parse_section(state, line + 1);
    }
    else
    {
        // all other non-empty and non-comment lines are data
        parse_data_line(chart, state, line);
    }
}
