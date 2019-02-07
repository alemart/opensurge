/*
 * Open Surge Engine
 * timer.c - time handler
 * Copyright (C) 2010, 2012  Alexandre Martins <alemartf(at)gmail(dot)com>
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



#include <allegro.h>
#include "global.h"
#include "timer.h"
#include "util.h"
#include "logfile.h"

#ifndef _WIN32
#include <sys/time.h>
#else
#include <winalleg.h>
#endif




/* constants */
#define MIN_FRAME_INTERVAL 15 /* (1/15) * 1000 ~ 67 fps max */
#define MAX_FRAME_INTERVAL 17 /* (1/17) * 1000 ~ 58 fps min */


/* internal data */
static int partial_fps, fps_accum, fps;
static uint32 last_time;
static float delta;
static int must_yield_cpu;
static volatile uint32 elapsed_time;
static uint32 start_time;

/* platform-specific code */
static uint32 get_tick_count(); /* tell me the time */
static void yield_cpu(); /* we don't like using 100% of the cpu */


/*
 * timer_init()
 * Initializes the Time Handler
 */
void timer_init(int optimize_cpu_usage)
{
    logfile_message("timer_init()");

    /* installing Allegro stuff */
    logfile_message("Installing Allegro timers...");
    if(install_timer() != 0)
        logfile_message("install_timer() failed: %s", allegro_error);

    /* should we optimize the cpu usage? */
    must_yield_cpu = optimize_cpu_usage;

    /* initializing... */
    partial_fps = 0;
    fps_accum = 0;
    fps = 0;
    delta = 0.0f;
    start_time = get_tick_count();

    /* done! */
    last_time = timer_get_ticks();
}


/*
 * timer_update()
 * Updates the Time Handler. This routine
 * must be called at every cycle of
 * the main loop
 */
void timer_update()
{
    uint32 current_time, delta_time; /* both in milliseconds */

    /* time control */
    for(delta_time = 0 ;;) {
        current_time = timer_get_ticks();
        delta_time = (current_time > last_time) ? (current_time - last_time) : 0;
        last_time = (current_time >= last_time) ? last_time : current_time;

        if(delta_time < MIN_FRAME_INTERVAL) {
            if(must_yield_cpu) {
                /* we don't like having the cpu usage at 100%. */
                /* will the OS make our process active again on time? */
                yield_cpu();
            }
        }
        else
            break;
    }
    delta_time = min(delta_time, MAX_FRAME_INTERVAL);
    delta = (float)delta_time * 0.001f;

    /* FPS (frames per second) */
    partial_fps++; /* 1 render per cycle */
    fps_accum += (int)delta_time;
    if(fps_accum >= 1000) {
        fps = partial_fps;
        partial_fps = 0;
        fps_accum = 0;
    }

    /* done! */
    last_time = timer_get_ticks();
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
 * Returns the time interval, in seconds,
 * between the last two cycles of the
 * main loop
 */
float timer_get_delta()
{
    return delta;
}


/*
 * timer_get_ticks()
 * Elapsed milliseconds since
 * the application has started
 */
uint32 timer_get_ticks()
{
    uint32 ticks = get_tick_count();
    if(ticks < start_time)
        start_time = ticks;
    return ticks - start_time;
}


/*
 * timer_get_fps()
 * Returns the FPS rate
 */
int timer_get_fps()
{
    return fps;
}

/*
 * timer_is_cpu_usage_optimized()
 * Is the CPU usage optimized?
 */
int timer_is_cpu_usage_optimized()
{
    return must_yield_cpu;
}

/*
 * timer_optimize_cpu_usage()
 * Optimize the CPU usage?
 */
void timer_optimize_cpu_usage(int optimize)
{
    must_yield_cpu = optimize;
}







/* platform-specific code */

#ifndef _WIN32

uint32 get_tick_count()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec * 1000) + (now.tv_usec / 1000);
}


void yield_cpu()
{
    rest(1); /* don't use rest(0) */
}

#else

uint32 get_tick_count()
{
    return GetTickCount();
}

void yield_cpu()
{
    Sleep(1);
}

#endif
