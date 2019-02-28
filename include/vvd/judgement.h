#pragma once

#include "screen.h"

// timing windows for each judgement, in milliseconds
// these windows are both before and after (+-)
// todo: unsure if near/error windows are correct
#define JUDGEMENT_CRITICAL_WINDOW 2 * SCREEN_FRAME_DURATION
#define JUDGEMENT_NEAR_WINDOW 4 * SCREEN_FRAME_DURATION
#define JUDGEMENT_ERROR_WINDOW 8 * SCREEN_FRAME_DURATION
#define JUDGEMENT_ANALOG_SLAM_WINDOW JUDGEMENT_CRITICAL_WINDOW + JUDGEMENT_NEAR_WINDOW

// timing window for the start of holds
// only applies to before (-)
// todo: whats the proper hold window?
#define JUDGEMENT_HOLD_START_WINDOW JUDGEMENT_ERROR_WINDOW

typedef enum
{
    JudgementNone,
    JudgementCritical,
    JudgementNear,
    JudgementError,
} Judgement;
