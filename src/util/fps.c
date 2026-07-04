/*
 * Open Surge Engine
 * fps.c - Framerate utility
 * Copyright 2008-2026 Alexandre Martins <alemartf(at)gmail.com>
 * http://opensurge2d.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "fps.h"
#include "numeric.h"

#define WANTED_METHOD 2
static double framerate = 0.0;
static double previous_time = 0.0;

/* method 1: count the number of frames */
#define UPDATES_PER_SECOND 1
static double last_update = 0.0;
static int counter = 0;

/* method 2: estimate the fps based on instantaneous framerates */
#define NUMBER_OF_SAMPLES TARGET_FPS
static double samples[NUMBER_OF_SAMPLES];
static int index_of_next_sample = 0;
static int outlier_count = 0;
static double min_framerate = 0.0;

/*
 * fps_init()
 * Initialize the FPS counter
 */
void fps_init()
{
    framerate = TARGET_FPS;
    previous_time = 0.0;

    last_update = 0.0;
    counter = 0;

    memset(samples, 0, NUMBER_OF_SAMPLES * sizeof(double));
    index_of_next_sample = 0;
    outlier_count = 0;
    min_framerate = TARGET_FPS;
}

/*
 * fps_release()
 * Release the FPS counter
 */
void fps_release()
{
    /* nothing to do */
}

/*
 * fps_update()
 * Update the FPS counter
 * Give as input the elapsed time, in seconds, at the beginning of the current framestep
 */
void fps_update(double elapsed_time)
{
#if WANTED_METHOD == 1

    /* method 1: count the number of frames
       method 1 typically gives 59, 60, 61. It's simple, but the timing is imprecise */
    ++counter;
    if(elapsed_time >= last_update + 1.0 / UPDATES_PER_SECOND) {
        last_update = elapsed_time;
        framerate = counter * UPDATES_PER_SECOND;
        min_framerate = framerate;
        counter = 0;
    }

#else

    /* method 2: invert the median of n samples of estimates of the inverse framerate
       method 2 typically gives 60 */
    if(index_of_next_sample >= NUMBER_OF_SAMPLES) {
        double median_delta, max_delta;
        outlier_count = find_outliers(samples, NUMBER_OF_SAMPLES, NULL, &median_delta, NULL, NULL, &max_delta);
        framerate = 1.0 / median_delta; /* an overall measure of the performance */
        min_framerate = 1.0 / max_delta; /* how reliable is this estimate? */
        index_of_next_sample = 0;
    }

    /* find the unscaled delta_time, in seconds */
    double delta_time = elapsed_time > previous_time ? elapsed_time - previous_time : 0.0;

    /* collect a sample of an estimate of the inverse framerate */
    samples[index_of_next_sample++] = delta_time;

#endif

    /* save the current time for the next frame */
    previous_time = elapsed_time;
}

/*
 * fps_current()
 * Read the current framerate
 */
double fps_current()
{
    return framerate;
}

/*
 * fps_stability()
 * A percentage of overall smoothness which is intended to capture micro-stutters
 */
double fps_stability()
{
    /* This percentage tells us "how many" micro-stutters we're not experiencing per window of time
       The closer to 100%, the better. Running at TARGET_FPS with 100% or near stability is the intended experience
       A low percentage, particularly when the game is paused, may indicate unrelated work of the operating system (or of other programs) */

    double stutter_rate = (double)outlier_count / NUMBER_OF_SAMPLES;
    return 1.0 - stutter_rate;
}

/*
 * fps_noise()
 * Another measure of smoothness intended to capture micro-stutters
 */
double fps_noise()
{
    /* This value measures "how bad" was the worst micro-stutter we've just experienced (if any)
       The closer to zero, the better. If fps_stability ~ 100%, then generally fps_noise ~ 0 (meaning: no stuttering)
       This value MAY reveal tasks like loading resources or heavy garbage collection, particularly if fps_stability is near 100%
       and fps_noise >> 0, but it can't be reliably separated from unrelated work of the operating system (or of other programs).
       This said, if fps_stability << 100% or fps_noise >> 0, then we're likely to be experiencing micro-stuttering */

    /* if we consider only the worst micro-stutter, this is the deviation from the framerate */
    return framerate - min_framerate; /* how accurate is this? */
}
