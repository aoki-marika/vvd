#include "interpolate.h"

double interpolate(double time, double start_time, double end_time, double start_value, double end_value)
{
    double percentage = (time - start_time) / (end_time - start_time);
    return start_value + percentage * (end_value - start_value);
}
