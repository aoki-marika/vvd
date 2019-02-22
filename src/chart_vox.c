#include "chart_vox.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "note_utils.h"

void *chart_vox_parsing_state_create()
{
    // create the parsing state
    VoxParsingState *state = malloc(sizeof(VoxParsingState));

    // set the default section to none
    state->section = VoxSectionNone;

    // default the building analogs to null
    for (int i = 0; i < CHART_ANALOG_LANES; i++)
        state->building_analogs[i] = NULL;

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
    // numerous early vox files have this typo
    else if (strcmp(name, "END POSITION") == 0 ||
             strcmp(name, "END POSISION") == 0)
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
                chart->num_measures = measure + 1;
                chart->end_subbeat = note_time_to_subbeat(chart, measure, beat, subbeat);

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
                    .subbeat = subbeat,
                };

                // if there are any beats before the current beat
                if (chart->num_beats > 0)
                {
                    // set the beats subbeat
                    // a subbeat cant be calculated without a beat already existing
                    Beat *note_beat = &chart->beats[chart->num_beats - 1];
                    chart_beat.subbeat = note_time_at_beat_to_subbeat(note_beat, measure, beat, subbeat);
                }

                // append the beat to the charts beats
                chart->beats[chart->num_beats] = chart_beat;
                chart->num_beats++;

                break;
            }
            case VoxSectionBpmInfo:
            {
                // timing, bpm, direction
                // not entirely sure how direction works but its typically 4, and for the twotorial stops its -4
                // twotorial stops sort of work in that the start and notes after are timed correctly but sdvx seems to just skip to the end and wait if its a stop
                // todo: booth vox dont have timing for bpm
                // e.g.
                // #BPM
                // 177.0000
                // #END
                assert(num_values == 3);

                // add the tempo to the given chart
                chart_add_tempo(chart, atof(values[1]), note_time_to_subbeat(chart, measure, beat, subbeat));

                break;
            }
            case VoxSectionTrackAnalogL:
            case VoxSectionTrackAnalogR:
            {
                // timing, position, state, spin, effect, wide, ? (only in newer charts)
                // position: 0 - 127
                // state: 0 = continue, 1 = start, 2 = end
                // spin:
                //  - sweeps are where the track starts spinning but then swings back
                //  - 1 = not sweep, 1 measure long
                //  - 2 = not sweep, 1/4 measure long
                //  - 3 = not sweep, 1/2 measure long
                //  - 4 = sweep, 1 measure long
                //  - 5 = sweep, 1/2 measure long
                //  - 6 = sweep, 1/4 measure long
                //  - 7 = sweep, 1/8 measure long
                // effect: laser effect number?
                // wide: 1 = normal, 2 = wide (maybe just a multiplier for position?)
                // ?: ?
                // slams are parsed by two points being on the same subbeat
                assert(num_values == 5 || num_values == 6 || num_values == 7);

                // get this points state
                int point_state = atoi(values[2]);

                // some vox have invalid point states, ignore those
                if (point_state != VoxAnalogStateContinue &&
                    point_state != VoxAnalogStateStart &&
                    point_state != VoxAnalogStateEnd)
                    return;

                // get the respective lane for the current section
                int lane;
                switch (state->section)
                {
                    case VoxSectionTrackAnalogL:
                        lane = CHART_ANALOG_LANE_L;
                        break;
                    case VoxSectionTrackAnalogR:
                        lane = CHART_ANALOG_LANE_R;
                        break;
                }

                // start an analog if this is a start point
                if (point_state == VoxAnalogStateStart)
                {
                    // assert that there is not already an analog being built
                    assert(!state->building_analogs[lane]);

                    // create a new analog and set the building analog to it
                    Analog *analog = malloc(sizeof(Analog));
                    analog->num_points = 0;
                    state->building_analogs[lane] = analog;
                }

                // assert that an analog is currently being built
                assert(state->building_analogs[lane]);

                // create the point
                AnalogPoint point = (AnalogPoint)
                {
                    .subbeat = note_time_to_subbeat(chart, measure, beat, subbeat),
                    .position = atoi(values[1]) / 127.0, //position is on a scale of 0 to 127
                    .position_scale = (num_values >= 6) ? atof(values[5]) : 1,
                    .slam = false,
                };

                // set whether or not the current point is a slam
                if (state->building_analogs[lane]->num_points > 0)
                {
                    AnalogPoint *previous_point = &state->building_analogs[lane]->points[state->building_analogs[lane]->num_points - 1];
                    point.slam = point.subbeat == previous_point->subbeat;
                }

                // append the point to the current building analogs points
                Analog *analog = state->building_analogs[lane];
                analog->points[analog->num_points] = point;
                analog->num_points += 1;

                // end the current analog if this is an end point
                if (point_state == VoxAnalogStateEnd)
                {
                    // pop the current analog into the charts analogs
                    Analog *analog = state->building_analogs[lane];
                    chart->analogs[lane][chart->num_analogs[lane]] = *analog;
                    chart->num_analogs[lane] += 1;

                    // free the now finished analog
                    free(analog);
                    state->building_analogs[lane] = NULL;
                }

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

                // create the note
                Note note = (Note)
                {
                    .start_subbeat = note_time_to_subbeat(chart, measure, beat, subbeat),
                };

                // set the hold properties if this note is a hold
                int length = atoi(values[1]);
                if (length > 0)
                {
                    note.hold = true;
                    note.end_subbeat = note.start_subbeat + length;
                }

                // append the note to the charts notes
                notes[*num_notes] = note;
                *num_notes += 1;

                break;
            }
        }
    }
}

void chart_vox_parse_line(Chart *chart, void *parsing_state, char *line)
{
    VoxParsingState *state = (VoxParsingState *)parsing_state;

    // ignore comment and blank lines
    // some old vox files have "sections" that are actually comments
    if (has_prefix("//", line) ||
        strcmp(line, "") == 0 ||
        strcmp(line, "#====================================") == 0 ||
        strcmp(line, "#ITEM") == 0 ||
        strcmp(line, "#LINE") == 0 ||
        strcmp(line, "#SONG") == 0 ||
        strcmp(line, "# SOUND VOLTEX OUTPUT TEXT FILE") == 0 ||
        strcmp(line, "# TRACK INFO") == 0)
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
