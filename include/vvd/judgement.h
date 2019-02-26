#pragma once

#include "screen.h"

// timing windows for each judgement, in milliseconds
// these windows are both before and after (+-)
// todo: unsure if near/error windows are correct
#define JUDGEMENT_CRITICAL_WINDOW 2 * SCREEN_FRAME_DURATION
#define JUDGEMENT_NEAR_WINDOW 4 * SCREEN_FRAME_DURATION
#define JUDGEMENT_ERROR_WINDOW 8 * SCREEN_FRAME_DURATION
#define JUDGEMENT_ANALOG_SLAM_WINDOW JUDGEMENT_CRITICAL_WINDOW + JUDGEMENT_NEAR_WINDOW

typedef enum
{
    JudgementNone,
    JudgementCritical,
    JudgementNear,
    JudgementError,
} Judgement;
