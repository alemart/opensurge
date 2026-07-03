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

#define UPDATE_FREQUENCY 1 /* updates per second (ideally) */
static int counter = 0;
static double counted_estimate = 0.0;
static double last_update = 0.0;

#define NUMBER_OF_SAMPLES (TARGET_FPS / UPDATE_FREQUENCY)
static double sample[NUMBER_OF_SAMPLES];
static int index_of_next_sample = 0;
static double sampled_estimate = 0.0;

/*
 * fps_init()
 * Initialize the FPS counter
 */
void fps_init()
{
    counter = 0;
    counted_estimate = 0.0;
    last_update = 0.0;

    sampled_estimate = 0.0;
    index_of_next_sample = 0;
    memset(sample, 0, NUMBER_OF_SAMPLES * sizeof(double));
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
 * Update the FPS counter. Call every frame with unscaled time
 */
void fps_update(double elapsed_time, double delta_time)
{
    /* method 1: count the number of frames
       method 1 typically gives 59, 60, 61. It's simple, but the timing is imprecise */
    ++counter;
    if(elapsed_time >= last_update + 1.0 / UPDATE_FREQUENCY) {
        last_update = elapsed_time;
        counted_estimate = counter * UPDATE_FREQUENCY;
        counter = 0;
    }

    /* method 2: invert the median of n samples of estimates of the inverse framerate
       method 2 typically gives 60 */
    if(index_of_next_sample >= NUMBER_OF_SAMPLES) {
        sampled_estimate = 1.0 / find_median(sample, NUMBER_OF_SAMPLES);
        index_of_next_sample = 0;
    }

    /* collect a sample */
    sample[index_of_next_sample++] = delta_time;
}

/*
 * fps_current()
 * Read the current framerate
 */
double fps_current()
{
    return sampled_estimate;
}
