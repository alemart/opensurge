/*
 * Open Surge Engine
 * screenshot.c - screenshots module
 * Copyright (C) 2008-2010  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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
#include "osspec.h"
#include "logfile.h"
#include "video.h"
#include "input.h"

/* private data */
#define MAX_SCREENSHOTS     (1 << 30)
static input_t *in;
static char *next_available_filename();

/*
 * screenshot_init()
 * Initializes the screenshot module
 */
void screenshot_init()
{
    in = input_create_user("screenshots");
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
        char *file = next_available_filename();
        image_save(video_get_window_surface(), file);
        video_showmessage("'screenshots/%s' saved", basename(file));
        logfile_message("New screenshot: %s", file);
    }
}


/*
 * screenshot_release()
 * Releases this module
 */
void screenshot_release()
{
    input_destroy(in);
}






/* misc */
char *next_available_filename()
{
    static char f[64], abs_path[1024];
    int i;

    for(i=0; i<MAX_SCREENSHOTS; i++) {
        sprintf(f, "screenshots/s%03d.png", i);
        resource_filepath(abs_path, f, sizeof(abs_path), RESFP_WRITE);
        if(!filepath_exists(abs_path))
            return abs_path;
    }

    return abs_path;
}
