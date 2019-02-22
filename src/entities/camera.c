/*
 * Open Surge Engine
 * camera.c - camera routines
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "camera.h"
#include "../core/util.h"
#include "../core/video.h"
#include "../core/timer.h"
#include "../scenes/level.h"

/* private stuff */
typedef struct camera_t camera_t;
struct camera_t {
    /* the camera is actually a particle */
    v2d_t position; /* current position */
    v2d_t dest; /* target position: used to make things smooth */
    float speed; /* the camera will move from position to dest in speed px/s */

    /* the camera is only allowed to see within the bounds of region[] */
    v2d_t region_topleft, region_bottomright; /* this describes a rectangle: current boundaries */
    v2d_t dest_region_topleft, dest_region_bottomright; /* target boundaries */
    float region_topleft_speed, region_bottomright_speed; /* speed, in px/s */

    /* misc */
    int is_locked; /* is the camera locked or can it move freely in the level? */
};

static camera_t camera;
static void define_boundaries(int x1, int y1, int x2, int y2);
static void update_boundaries();



/* public methods */

/*
 * camera_init()
 * initializes the camera
 */
void camera_init()
{
    camera.is_locked = FALSE;

    camera.speed = 0.0f;
    camera.region_topleft_speed = 0.0f;
    camera.region_bottomright_speed = 0.0f;

    camera.position = camera.dest = v2d_new(0.0f, 0.0f);
    camera.region_topleft.x = camera.dest_region_topleft.x = VIDEO_SCREEN_W/2;
    camera.region_topleft.y = camera.dest_region_topleft.y = VIDEO_SCREEN_H/2;
    camera.region_bottomright.x = camera.dest_region_bottomright.x = level_size().x-VIDEO_SCREEN_W/2;
    camera.region_bottomright.y = camera.dest_region_bottomright.y = level_size().y-VIDEO_SCREEN_H/2;
}

/*
 * camera_update()
 * updates the camera
 */
void camera_update()
{
    const float threshold = 10.0f;
    float dt = timer_get_delta();
    v2d_t ds;

    /* the level size may have changed during the last frame */
    update_boundaries();

    /* updating the camera position */
    ds = v2d_subtract(camera.dest, camera.position);
    if(v2d_magnitude(ds) > threshold) {
        ds = v2d_normalize(ds);
        camera.position.x += ds.x * camera.speed * dt;
        camera.position.y += ds.y * camera.speed * dt;
    }

    /* updating the feasible region */
    ds = v2d_subtract(camera.dest_region_topleft, camera.region_topleft);
    if(v2d_magnitude(ds) > threshold) {
        ds = v2d_normalize(ds);
        camera.region_topleft.x += ds.x * camera.region_topleft_speed * dt;
        camera.region_topleft.y += ds.y * camera.region_topleft_speed * dt;
    }

    ds = v2d_subtract(camera.dest_region_bottomright, camera.region_bottomright);
    if(v2d_magnitude(ds) > threshold) {
        ds = v2d_normalize(ds);
        camera.region_bottomright.x += ds.x * camera.region_bottomright_speed * dt;
        camera.region_bottomright.y += ds.y * camera.region_bottomright_speed * dt;
    }

    /* clipping... */
    camera.position.x = clip(camera.position.x, camera.region_topleft.x, camera.region_bottomright.x);
    camera.position.y = clip(camera.position.y, camera.region_topleft.y, camera.region_bottomright.y);
}

/*
 * camera_release()
 * releases the camera
 */
void camera_release()
{
    ; /* empty */
}

/*
 * camera_move_to()
 * moves the camera to a new position within a few seconds
 */
void camera_move_to(v2d_t position, float seconds)
{
    /* clipping */
    if(position.x < camera.region_topleft.x)
        position.x = camera.region_topleft.x;

    if(position.y < camera.region_topleft.y)
        position.y = camera.region_topleft.y;

    if(position.x > camera.region_bottomright.x)
        position.x = camera.region_bottomright.x;

    if(position.y > camera.region_bottomright.y)
        position.y = camera.region_bottomright.y;

    /* updating the target position */
    camera.dest = position;

    /* hey, don't move too fast! */
    if(seconds > 0.016f)
        camera.speed = v2d_magnitude( v2d_subtract(camera.position, camera.dest) ) / seconds;
    else
        camera.position = camera.dest;

}

/*
 * camera_lock()
 * locks the camera, so it will only move within the given rectangle (in pixels)
 */
void camera_lock(int x1, int y1, int x2, int y2)
{
    camera.is_locked = TRUE;
    define_boundaries(x1, y1, x2, y2);
}

/*
 * camera_unlock()
 * unlocks the camera, so it will move freely in the level
 */
void camera_unlock()
{
    camera.is_locked = FALSE;
}

/*
 * camera_get_position()
 * returns the position of the camera
 */
v2d_t camera_get_position()
{
    return v2d_new( (int)camera.position.x, (int)camera.position.y );
}

/*
 * camera_set_position()
 * sets a new position
 */
void camera_set_position(v2d_t position)
{
    if(!level_editmode()) {
        camera.dest.x = clip(position.x, VIDEO_SCREEN_W/2, level_size().x - VIDEO_SCREEN_W/2);
        camera.dest.y = clip(position.y, VIDEO_SCREEN_H/2, level_size().y - VIDEO_SCREEN_H/2);
        camera.position = camera.dest;
    }
    else
        camera.position = camera.dest = position;
}

/*
 * camera_is_locked()
 * Is the camera locked?
 */
int camera_is_locked()
{
    return camera.is_locked;
}

/* private methods */
void define_boundaries(int x1, int y1, int x2, int y2)
{
    float seconds = 0.25f;

    camera.dest_region_topleft.x = max(min(x1, x2), VIDEO_SCREEN_W/2);
    camera.dest_region_topleft.y = max(min(y1, y2), VIDEO_SCREEN_H/2);
    camera.dest_region_bottomright.x = min(max(x1, x2), level_size().x-VIDEO_SCREEN_W/2);
    camera.dest_region_bottomright.y = min(max(y1, y2), level_size().y-VIDEO_SCREEN_H/2);

    camera.region_topleft_speed = v2d_magnitude (
        v2d_subtract( camera.region_topleft, camera.dest_region_topleft )
    ) / seconds;

    camera.region_bottomright_speed = v2d_magnitude (
        v2d_subtract( camera.region_bottomright, camera.dest_region_bottomright )
    ) / seconds;
}

void update_boundaries()
{
    if(!camera.is_locked)
        define_boundaries(-LARGE_INT, -LARGE_INT, LARGE_INT, LARGE_INT);
}
