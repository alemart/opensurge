/*
 * Open Surge Engine
 * screenshot.c - screenshots module
 * Copyright (C) 2008-2010, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <stdio.h>
#include "screenshot.h"
#include "assetfs.h"
#include "logfile.h"
#include "image.h"
#include "video.h"
#include "input.h"

/* private data */
#define MAX_SCREENSHOTS     (1 << 30)
static input_t *in;
static const char* screenshot_filename(int screenshot_id);
static int next_screenshot_id = 0;

/*
 * screenshot_init()
 * Initializes the screenshot module
 */
void screenshot_init()
{
    /* Create the input object */
    in = input_create_user("screenshots");

    /* What's the next screenshot? */
    while(assetfs_exists(screenshot_filename(next_screenshot_id)) &&
    ++next_screenshot_id < MAX_SCREENSHOTS);
}


/*
 * screenshot_update()
 * Checks if the user wants to take a snapshot, and if
 * he/she does, we must do it.
 */
void screenshot_update()
{
    /* take the snapshot! (press the '=' key or the 'printscreen' key) */
    if(input_button_pressed(in, IB_FIRE1) || input_button_pressed(in, IB_FIRE2)) {
        const char *filename = screenshot_filename(next_screenshot_id++);
        image_save(video_get_window_surface(), filename);
        video_showmessage("New screenshot: %s", filename);
        logfile_message("New screenshot: \"%s\"", filename);
    }
}


/*
 * screenshot_release()
 * Releases this module
 */
void screenshot_release()
{
    /* We're done with the input object */
    input_destroy(in);
}






/* misc */
const char* screenshot_filename(int screenshot_id)
{
    static char filename[32];
    snprintf(filename, sizeof(filename), "screenshots/s%03d.png", screenshot_id);
    return filename;
}