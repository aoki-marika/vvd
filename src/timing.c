#include "timing.h"

#include <stdlib.h>
#include <sys/time.h>

double time_milliseconds()
{
    // get the current time of day
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // convert tv_sec and tv_usec to ms and return
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}
