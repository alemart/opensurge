/*
 * Open Surge Engine
 * timer.c - time manager
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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

#include "util.h"
#include "timer.h"
#include "logfile.h"

#include <allegro5/allegro.h>

/* internal data */
static double start_time = 0.0;
static double current_time = 0.0;
static float delta_time = 0.0f;
static int64_t frames = 0;

/*
 * timer_init()
 * Initializes the Time Handler
 */
void timer_init()
{
    logfile_message("timer_init()");

    /* Allegro must be initialized before we can call al_get_time() */
    if(!al_is_system_installed())
        fatal_error("Allegro is not initialized");

    start_time = al_get_time();
    current_time = 0.0f;
    delta_time = 0.0;
    frames = 0;
}


/*
 * timer_update()
 * Updates the Time Handler. This routine
 * must be called at every cycle of the
 * main loop
 */
void timer_update()
{
    static const float minimum_delta = 0.0166667f; /* 60 fps */
    static const float maximum_delta = 0.017f; /* if 0.0166667f, you don't get physics with the fixed timestep; if too large (0.20f), there may be issues with collisions */
    static double old_time = 0.0;

    /* read the time at the beginning of this framestep */
    current_time = al_get_time() - start_time;

    /* compute the delta time */
    delta_time = current_time - old_time;

    if(delta_time < minimum_delta)
        ; /* do nothing, since the framerate is controlled by Allegro */

    if(delta_time > maximum_delta)
        delta_time = maximum_delta;

    old_time = current_time;

    /* increment counter */
    ++frames;
}


/*
 * timer_release()
 * Releases the Time Handler
 */
void timer_release()
{
    logfile_message("timer_release()");
}


/*
 * timer_get_delta()
 * Returns the time interval, in seconds, between the
 * last two cycles of the main loop
 */
float timer_get_delta()
{
    return delta_time;
}


/*
 * timer_get_ticks()
 * Elapsed milliseconds since the application has started,
 * measured at the beginning of the current framestep
 */
uint32_t timer_get_ticks()
{
    return (uint32_t)(1000.0 * current_time);
}


/*
 * timer_get_elapsed()
 * Elapsed seconds since the application has started,
 * measured at the beginning of the current framestep
 */
float timer_get_elapsed()
{
    return (float)current_time;
}


/*
 * timer_get_frames()
 * Number of framesteps since the application has started
 */
int64_t timer_get_frames()
{
    return frames;
}


/*
 * timer_get_now()
 * Elapsed of seconds since the application has started
 * and at the moment of the function call
 */
float timer_get_now()
{
    return (float)(al_get_time() - start_time);
}