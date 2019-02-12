#include "chart_ksh.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

void *chart_ksh_parsing_state_create()
{
    KSHParsingState *state = malloc(sizeof(KSHParsingState));

    // init default values
    state->metadata = true;
    state->measure = 0;
    state->num_measure_lines = 0;
    state->num_measure_notes = 0;

    // allocate arrays
    for (int i = 0; i < KSH_MEASURE_LINES_MAX; i++)
        state->measure_lines[i] = malloc(CHART_STR_MAX * sizeof(char));

    // return the state
    return state;
}

void chart_ksh_parsing_state_free(void *parsing_state)
{
    KSHParsingState *state = (KSHParsingState *)parsing_state;

    // free measure lines
    for (int i = 0; i < KSH_MEASURE_LINES_MAX; i++)
        free(state->measure_lines[i]);

    // free the state
    free(state);
}

// returns whether or not the given line is a k/v pair
bool line_is_kv_pair(char *line)
{
    // k/v pairs are formatted as "key=value"
    return strchr(line, '=');
}

// add a new beat to the given chart
void add_beat(Chart *chart,
              char *value,
              uint16_t measure,
              uint8_t beat,
              uint8_t subbeat)
{
    // create the beat
    // beats are 'numerator/denominator'
    Beat chart_beat = (Beat)
    {
        .numerator = atoi(strtok(value, "/")),
        .denominator = atoi(strtok(NULL, "/")),
        .measure = measure,
        .beat = beat,
        .subbeat = subbeat,
    };

    // append it to the charts beats
    chart->beats[chart->num_beats] = chart_beat;
    chart->num_beats++;
}

// add a new tempo to the given chart
void add_tempo(Chart *chart,
               char *value,
               uint16_t measure,
               uint8_t beat,
               uint8_t subbeat)
{
    // create the tempo
    // tempos are just a float value
    Tempo tempo = (Tempo)
    {
        .bpm = atof(value),
        .measure = measure,
        .beat = beat,
        .subbeat = subbeat,
    };

    // append it to the charts tempos
    chart->tempos[chart->num_tempos] = tempo;
    chart->num_tempos++;
}

// parse a k/v pair and assign it to the respective value in the given chart
void parse_kv_pair(Chart *chart,
                   char *line,
                   uint16_t measure,
                   uint8_t beat,
                   uint8_t subbeat)
{
    // assert that this line is a k/v pair
    assert(line_is_kv_pair(line));

    // parse out the key and value
    char *key = strtok(line, "=");
    char *value = strtok(NULL, "=");
    char *value_not_null = value ? value : ""; //strcpy doesnt like null values

    // assign the value to the respective property on the given chart

    // title
    if (strcmp(key, "title") == 0)
        strcpy(chart->title, value_not_null);
    // artist
    else if (strcmp(key, "artist") == 0)
        strcpy(chart->artist, value_not_null);
    // effector
    else if (strcmp(key, "effect") == 0)
         strcpy(chart->effector, value_not_null);
    // illustrator
    else if (strcmp(key, "illustrator") == 0)
        strcpy(chart->illustrator, value_not_null);
    // rating
    else if (strcmp(key, "level") == 0)
        chart->rating = atoi(value);
    // offset
    else if (strcmp(key, "o") == 0)
        chart->offset = atof(value);
    // beat
    else if (strcmp(key, "beat") == 0)
        add_beat(chart, value, measure, beat, subbeat);
    // tempo
    else if (strcmp(key, "t") == 0)
        add_tempo(chart, value, measure, beat, subbeat);
    // print out unhandled k/v pairs for debugging
    else
        printf("parse_kv_pair: unhandled key/value pair '%s':'%s'\n", key, value);
}

void parse_notes(int num_lanes, // the number of lanes for the note type to parse
                 char *values, //the note values from the note line for this type, e.g. the '0000' of '0000|00|00' for bt notes
                 Note *processing_holds[num_lanes], //the currently processing hold notes for the note type to parse
                 int num_notes[num_lanes], //the number of items in each lane of notes, incremented when new notes are added
                 Note *notes[num_lanes], //the notes from chart of the note type to insert new notes into
                 uint16_t measure,
                 uint8_t beat,
                 uint8_t subbeat)
{
    // for each lane
    for (int i = 0; i < num_lanes; i++)
    {
        // 0 is none for both types
        bool none = values[i] == '0';

        // ksh oddity
        // bt notes use 1 for chip, and 2 for hold
        // fx notes use 1 for hold, and 2 for chip
        bool chip = num_lanes == CHART_FX_LANES ? values[i] == '2' : values[i] == '1';
        bool hold = num_lanes == CHART_FX_LANES ? values[i] == '1' : values[i] == '2';

        // if this is a chip or hold note and theres no currently processing hold note
        if (chip || (hold && !processing_holds[i]))
        {
            // this is the start of a new note

            // create the note
            Note note = (Note)
            {
                .start_measure = measure,
                .start_beat = beat,
                .start_subbeat = subbeat,
                .hold = hold,
            };

            // append it to notes
            notes[i][num_notes[i]] = note;
            num_notes[i]++;

            // store a pointer in processing_holds to note if this is a hold note
            if (hold)
                processing_holds[i] = &notes[i][num_notes[i] - 1];
        }
        // if there is no note and there is a processing hold on this lane
        else if (none && processing_holds[i])
        {
            // this is the end of the currently processing hold note on this lane

            // get the processing hold
            Note *hold = processing_holds[i];

            // set the end timing properties
            hold->end_measure = measure;
            hold->end_beat = beat;
            hold->end_subbeat = subbeat;

            // drop it from processing_holds
            processing_holds[i] = NULL;
        }
    }
}

void parse_measure(Chart *chart,
                   KSHParsingState *state)
{
    // the last beat and subbeat that was parsed
    uint8_t last_beat = 0;
    uint8_t last_subbeat = 0;

    // for each line in the current measure
    int n = 0; //the current note line index
    for (int i = 0; i < state->num_measure_lines; i++)
    {
        char *line = state->measure_lines[i];

        // if the current line is a k/v pair
        if (line_is_kv_pair(line))
        {
            // parse the k/v pair with the current measure and last beat/subbeat
            parse_kv_pair(chart,
                          line,
                          state->measure,
                          last_beat,
                          last_subbeat);
        }
        // if the current line is a note
        else
        {
            // parse out the individual note type values
            char *bt = strtok(line, "|");
            char *fx = strtok(NULL, "|");
            char *lasers = strtok(NULL, "|");

            // assert that this line is a valid note line
            assert(bt && fx && lasers);

            // get references to commonly used values
            Beat *beat = &chart->beats[chart->num_beats - 1];
            float numerator = beat->numerator;
            float denominator = beat->denominator;

            // calculate the beat and subbeat
            float subbeat_per_measure = CHART_BEAT_MAX_SUBBEATS * 4 / denominator * numerator;
            float subbeat_per_line = subbeat_per_measure / (float)state->num_measure_notes;
            float current_line_subbeat = subbeat_per_line * (float)n;
            last_beat = floor(current_line_subbeat / (CHART_BEAT_MAX_SUBBEATS * 4 / denominator));
            last_subbeat = fmod(current_line_subbeat, CHART_BEAT_MAX_SUBBEATS * 4 / denominator);

            // parse the notes of the current line
            parse_notes(CHART_BT_LANES,
                        bt,
                        state->processing_bt_holds,
                        chart->num_bt_notes,
                        chart->bt_notes,
                        state->measure,
                        last_beat,
                        last_subbeat);

            parse_notes(CHART_FX_LANES,
                        fx,
                        state->processing_fx_holds,
                        chart->num_fx_notes,
                        chart->fx_notes,
                        state->measure,
                        last_beat,
                        last_subbeat);

            // increment the current note index
            n++;
        }
    }
}

void chart_ksh_parse_line(Chart *chart, void *parsing_state, char *line)
{
    KSHParsingState *state = (KSHParsingState *)parsing_state;

    // if parsing metadata and the current line is a k/v pair
    if (state->metadata && line_is_kv_pair(line))
    {
        // parse the k/v pair with a start time of 0/0/0
        parse_kv_pair(chart,
                      line,
                      0,
                      0,
                      0);
    }
    // if the current line is a measure start
    else if (strcmp(line, "--") == 0)
    {
        // parse the last measure if it had any lines
        if (state->num_measure_lines > 0)
            parse_measure(chart, state);

        // increment the current measure if not in metdata, else say parsing is no longer in metadata
        // this is to handle using an unsigned int for measure while having the first measure be zero
        if (!state->metadata)
            state->measure++;
        else
            state->metadata = false;

        // reset all the per-measure counts
        state->num_measure_lines = 0;
        state->num_measure_notes = 0;
    }
    // if the current line is part of a measure
    else
    {
        // copy the current line to the end of state->measure_lines
        strcpy(state->measure_lines[state->num_measure_lines], line);
        state->num_measure_lines++;

        // increment state->num_measure_notes if the current line is a note
        // all of the lines handled by this function should be either k/v pairs or notes
        if (!line_is_kv_pair(line))
            state->num_measure_notes++;
    }
}
