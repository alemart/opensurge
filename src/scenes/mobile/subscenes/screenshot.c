/*
 * Open Surge Engine
 * screenshot.c - screenshot subscene for mobile devices
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

#include "screenshot.h"
#include "../../../core/image.h"
#include "../../../core/util.h"

typedef struct mobile_subscene_screenshot_t mobile_subscene_screenshot_t;
struct mobile_subscene_screenshot_t {
    mobile_subscene_t super;
    const image_t* screenshot;
};

static void init(mobile_subscene_t*);
static void release(mobile_subscene_t*);
static void update(mobile_subscene_t*,v2d_t);
static void render(mobile_subscene_t*,v2d_t);
static const mobile_subscene_t super = { .init = init, .release = release, .update = update, .render = render };




/*
 * mobile_subscene_screenshot()
 * Returns a new instance of the screenshot subscene
 */
mobile_subscene_t* mobile_subscene_screenshot(const image_t* screenshot)
{
    mobile_subscene_screenshot_t* subscene = mallocx(sizeof *subscene);

    subscene->super = super;
    subscene->screenshot = screenshot;

    return (mobile_subscene_t*)subscene;
}






/*
 * init()
 * Initializes the subscene
 */
void init(mobile_subscene_t* subscene_ptr)
{
    (void)subscene_ptr;
}

/*
 * release()
 * Releases the subscene
 */
void release(mobile_subscene_t* subscene_ptr)
{
    mobile_subscene_screenshot_t* subscene = (mobile_subscene_screenshot_t*)subscene_ptr;

    free(subscene);
}

/*
 * update()
 * Updates the subscene
 */
void update(mobile_subscene_t* subscene_ptr, v2d_t subscene_offset)
{
    (void)subscene_ptr;
    (void)subscene_offset;
}

/*
 * render()
 * Renders the subscene
 */
void render(mobile_subscene_t* subscene_ptr, v2d_t subscene_offset)
{
    mobile_subscene_screenshot_t* subscene = (mobile_subscene_screenshot_t*)subscene_ptr;

    int width = image_width(subscene->screenshot);
    int height = image_height(subscene->screenshot);

    int x = subscene_offset.x;
    int y = subscene_offset.y;

    image_blit(subscene->screenshot, 0, 0, x, y, width, height);
}