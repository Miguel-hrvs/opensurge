/*
 * Open Surge Engine
 * timer.c - time manager
 * Copyright 2008-2024 Alexandre Martins <alemartf(at)gmail.com>
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

#include <allegro5/allegro.h>
#include "timer.h"
#include "logfile.h"
#include "../util/util.h"

/* internal data */
static double start_time = 0.0;
static double current_time = 0.0;
static double delta_time = 0.0;
static double smooth_delta_time = 0.0;
static int64_t frames = 0;

static bool is_paused = false;
static double pause_duration = 0.0;
static double pause_start_time = 0.0;

static const double SMOOTH_FACTOR = 0.95;
static const double MINIMUM_DELTA = 1.0 / 60.0; /* 60 fps */
static const double MAXIMUM_DELTA = 1.0 / 50.0; /* 50 fps */

/*
 * timer_init()
 * Initializes the time manager
 */
void timer_init()
{
    logfile_message("timer_init()");

    /* Allegro must be initialized before we can call al_get_time() */
    if(!al_is_system_installed())
        fatal_error("Allegro is not initialized");

    start_time = al_get_time();
    current_time = 0.0;
    delta_time = 0.0;
    smooth_delta_time = 0.0;
    frames = 0;

    is_paused = false;
    pause_duration = 0.0;
    pause_start_time = 0.0;
}

/*
 * timer_release()
 * Releases the time manager
 */
void timer_release()
{
    logfile_message("timer_release()");
}

/*
 * timer_update()
 * This routine must be called at every cycle of the main loop
 */
void timer_update()
{
    /* Paused timer? Return early */
    if(is_paused) {
        delta_time = 0.0;
        smooth_delta_time = 0.0;
        return;
    }

    /* Read the current time */
    double new_time = al_get_time();

    /* Compute the delta time */
    delta_time = new_time - current_time;

    /* Clamp delta time to a reasonable range */
    if(delta_time < MINIMUM_DELTA)
        delta_time = MINIMUM_DELTA;
    else if(delta_time > MAXIMUM_DELTA)
        delta_time = MAXIMUM_DELTA;

    /* Update the current time */
    current_time = new_time;

    /* Compute the smooth delta time */
    if(smooth_delta_time != 0.0)
        smooth_delta_time = SMOOTH_FACTOR * delta_time + (1.0 - SMOOTH_FACTOR) * smooth_delta_time;
    else
        smooth_delta_time = MINIMUM_DELTA;

    /* Increment counter */
    ++frames;
}

/*
 * timer_get_delta()
 * Returns the time interval, in seconds, between the last two cycles of the main loop
 */
double timer_get_delta()
{
    return (double)delta_time;
}

/*
 * timer_get_smooth_delta()
 * An approximation of timer_get_delta() with variations smoothed out
 */
double timer_get_smooth_delta()
{
    return (double)smooth_delta_time;
}

/*
 * timer_get_elapsed()
 * Elapsed seconds since the application has started,
 * measured at the beginning of the current framestep
 */
double timer_get_elapsed()
{
    return current_time;
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
double timer_get_now()
{
    return (al_get_time() - start_time) - pause_duration;
}

/*
 * timer_pause()
 * Pauses the time manager
 */
void timer_pause()
{
    if(is_paused)
        return;

    is_paused = true;
    pause_start_time = al_get_time();

    logfile_message("The time manager has been paused");
}

/*
 * timer_resume()
 * Resumes the time manager
 */
void timer_resume()
{
    if(!is_paused)
        return;

    pause_duration += al_get_time() - pause_start_time;
    is_paused = false;

    logfile_message("The time manager has been resumed");
}
