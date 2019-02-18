#pragma once

#include <stdint.h>

#include "chart.h"

// calculate the subbeat of a note at the given beat and timing
uint16_t note_time_at_beat_to_subbeat(Beat *note_beat, uint16_t measure, uint8_t beat, uint8_t subbeat);

// calculate the subbeat of a note in the given chart at the given timing
uint16_t note_time_to_subbeat(Chart *chart, uint16_t measure, uint8_t beat, uint8_t subbeat);
