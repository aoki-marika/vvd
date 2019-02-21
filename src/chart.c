#include "chart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "chart_ksh.h"
#include "chart_vox.h"
#include "note_utils.h"

void chart_parse_file(Chart *chart,
                      const char *path,
                      void *(* parsing_state_create)(),
                      void (* parsing_state_free)(void *),
                      void (* parse_line)(Chart *, void *, char *))
{
    // open the chart file for reading
    FILE *file = fopen(path, "r");

    // assert that the file is opened
    assert(file);

    // read each line and pass it into parse_line
    char line[CHART_STR_MAX];
    void *parsing_state = parsing_state_create();

    while (fgets(line, CHART_STR_MAX, file))
    {
        // cleanup the line by removing newlines, carriage returns, and boms
        char *position;

        // remove newlines
        if ((position = strchr(line, '\r')))
            *position = '\0';

        // remove carriage returns
        if ((position = strchr(line, '\n')))
            *position = '\0';

        // skip the bom if there is one
        int offset = 0;

        if (line[0] == 0xEF &&
            line[1] == 0xBB &&
            line[2] == 0xBF)
            offset = 3;

        // parse the line
        parse_line(chart, parsing_state, line + offset);
    }

    // free the parsing state and close the file
    parsing_state_free(parsing_state);
    fclose(file);
}

Chart *chart_create(const char *path)
{
    Chart *chart = malloc(sizeof(Chart));

    // default the offset and count properties to zero
    chart->offset = 0;
    chart->num_beats = 0;
    chart->num_tempos = 0;

    // allocate all the strings
    chart->title = malloc(CHART_STR_MAX * sizeof(char));
    chart->artist = malloc(CHART_STR_MAX * sizeof(char));
    chart->effector = malloc(CHART_STR_MAX * sizeof(char));
    chart->illustrator = malloc(CHART_STR_MAX * sizeof(char));

    // allocate all the events
    chart->beats = malloc(CHART_EVENTS_MAX * sizeof(Beat));
    chart->tempos = malloc(CHART_EVENTS_MAX * sizeof(Tempo));

    // allocate all the notes
    for (int i = 0; i < CHART_BT_LANES; i++)
    {
        chart->num_bt_notes[i] = 0;
        chart->bt_notes[i] = malloc(CHART_NOTES_MAX * sizeof(Note));
    }

    for (int i = 0; i < CHART_FX_LANES; i++)
    {
        chart->num_fx_notes[i] = 0;
        chart->fx_notes[i] = malloc(CHART_NOTES_MAX * sizeof(Note));
    }

    for (int i = 0; i < CHART_ANALOG_LANES; i++)
    {
        chart->num_analogs[i] = 0;
        chart->analogs[i] = malloc(CHART_NOTES_MAX * sizeof(Analog));
    }

    // get the proper chart parsing methods for the given path
    const char *path_extension = strrchr(path, '.');
    void *(* parsing_state_create)();
    void (* parsing_state_free)(void *);
    void (* parse_line)(Chart *, void *, char *);

    // if there is a path extension
    if (path_extension)
    {
        // if more chart types are added this is where their detection code would go
        if (strcmp(path_extension, ".ksh") == 0)
        {
            parsing_state_create = chart_ksh_parsing_state_create;
            parsing_state_free = chart_ksh_parsing_state_free;
            parse_line = chart_ksh_parse_line;
        }
        else if (strcmp(path_extension, ".vox") == 0)
        {
            parsing_state_create = chart_vox_parsing_state_create;
            parsing_state_free = chart_vox_parsing_state_free;
            parse_line = chart_vox_parse_line;
        }
    }

    // assert that a chart parsing method was found
    assert(parsing_state_create && parsing_state_free && parse_line);

    // parse the file
    chart_parse_file(chart, path, parsing_state_create, parsing_state_free, parse_line);

    // return the loaded chart
    return chart;
}

void chart_free(Chart *chart)
{
    // free all the strings
    free(chart->title);
    free(chart->artist);
    free(chart->effector);
    free(chart->illustrator);

    // free all the events
    free(chart->beats);
    free(chart->tempos);

    // free all the notes
    for (int i = 0; i < CHART_BT_LANES; i++)
        free(chart->bt_notes[i]);

    for (int i = 0; i < CHART_FX_LANES; i++)
        free(chart->fx_notes[i]);

    for (int i = 0; i < CHART_ANALOG_LANES; i++)
        free(chart->analogs[i]);

    // free the chart
    free(chart);
}

void chart_add_tempo(Chart *chart, double bpm, uint16_t subbeat)
{
    // create the tempo
    Tempo tempo = (Tempo)
    {
        .bpm = bpm,
        .time = 0,
        .subbeat = subbeat,
    };

    // if there any any tempos before the current tempo
    if (chart->num_tempos > 0)
    {
        // a tempo time cannot be calculated without a previous tempo to relatively time against
        // calculate the duration of the tempo
        Tempo *previous_tempo = &chart->tempos[chart->num_tempos - 1];
        double duration = subbeats_at_tempo_to_duration(previous_tempo, tempo.subbeat - previous_tempo->subbeat);

        // set the tempos time to the duration offset by the previous tempos time
        tempo.time = previous_tempo->time + duration;
    }

    // append the tempo to the given charts tempos
    chart->tempos[chart->num_tempos] = tempo;
    chart->num_tempos++;
}
