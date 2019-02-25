#pragma once

#include "screen.h"

// timing windows for each judgement, in milliseconds
// these windows are both before and after (+-)
// todo: unsure if near/error windows are correct
#define SCORING_CRITICAL_WINDOW 2 * SCREEN_FRAME_DURATION
#define SCORING_NEAR_WINDOW 4 * SCREEN_FRAME_DURATION
#define SCORING_ERROR_WINDOW 8 * SCREEN_FRAME_DURATION
#define SCORING_ANALOG_SLAM_WINDOW SCORING_CRITICAL_WINDOW + SCORING_NEAR_WINDOW
