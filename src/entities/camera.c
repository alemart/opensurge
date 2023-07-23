/*
 * Open Surge Engine
 * camera.c - camera routines
 * Copyright (C) 2008-2023  Alexandre Martins <alemartf@gmail.com>
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

#include <math.h>
#include "camera.h"
#include "../util/util.h"
#include "../core/video.h"
#include "../core/timer.h"
#include "../scenes/level.h"

/* private stuff */
typedef struct camera_t camera_t;
struct camera_t {
    /* the camera is represented by a point in 2D space */
    v2d_t position; /* current position, mapped to the center of the screen */
    v2d_t target; /* the target position is used to make things smooth */
    float speed; /* the camera will move from position to target in speed px/s */

    /* camera boundaries */
    struct {
        float x1, y1, x2, y2; /* x1 <= x2, y1 <= y2 */
        bool enabled; /* without boundaries, the camera travels through infinity */
    } boundaries;

    /* locking the camera */
    bool is_locked; /* is the camera locked or can it move freely? */
};

static camera_t camera;
static inline void define_boundaries(float x1, float y1, float x2, float y2);
static inline void reset_boundaries();
static inline void sanitize_boundaries();
static inline void enable_boundaries();
static inline void disable_boundaries();
static inline v2d_t clip_to_boundaries(v2d_t position);
static v2d_t clip_position(v2d_t position, float x1, float y1, float x2, float y2);



/* public methods */

/*
 * camera_init()
 * initializes the camera
 */
void camera_init()
{
    camera.is_locked = false;
    reset_boundaries();
    camera.position = v2d_new(camera.boundaries.x1, camera.boundaries.y1);
    camera.target = camera.position;
    camera.speed = 0.0f;
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

    /* updating the boundaries */
    if(level_editmode() || level_is_in_debug_mode()) /* no boundaries in the editor */
        disable_boundaries();
    else if(!camera.is_locked) /* the level size may have changed since the last frame */
        reset_boundaries();
    else
        enable_boundaries();

    /* updating the camera position */
    ds = v2d_subtract(camera.target, camera.position);
    if(v2d_magnitude(ds) > threshold) {
        ds = v2d_normalize(ds);
        camera.position.x += ds.x * camera.speed * dt;
        camera.position.y += ds.y * camera.speed * dt;
    }

    /* clipping... */
    camera.position = clip_to_boundaries(camera.position);
}

/*
 * camera_release()
 * releases the camera
 */
void camera_release()
{
    camera_unlock();
    disable_boundaries();
}

/*
 * camera_move_to()
 * moves the camera to a new position within a few seconds
 */
void camera_move_to(v2d_t position, float seconds)
{
    /* updating the target position */
    camera.target = clip_to_boundaries(position);

    /* hey, don't move too fast! */
    if(seconds > 0.016f)
        camera.speed = v2d_magnitude( v2d_subtract(camera.position, camera.target) ) / seconds;
    else
        camera.position = camera.target;
}

/*
 * camera_lock()
 * locks the camera, so it can only render points inside the given rectangle
 */
void camera_lock(int x1, int y1, int x2, int y2)
{
    int max_x = max(level_size().x - 1, 0);
    int max_y = max(level_size().y - 1, 0);
    int left = max(min(x1, x2), 0);
    int top = max(min(y1, y2), 0);
    int right = min(max(x1, x2), max_x);
    int bottom = min(max(y1, y2), max_y);

    /* not enough space? */
    if(right < left + VIDEO_SCREEN_W) {
        int m = (left + right + 1) / 2;
        left = m - VIDEO_SCREEN_W / 2;
        right = m + VIDEO_SCREEN_W / 2;
    }
    if(bottom < top + VIDEO_SCREEN_H) {
        int m = (top + bottom + 1) / 2;
        top = m - VIDEO_SCREEN_H / 2;
        bottom = m + VIDEO_SCREEN_H / 2;
    }

    /* lock & set boundaries */
    camera.is_locked = true;
    define_boundaries(
        left + VIDEO_SCREEN_W / 2,
        top + VIDEO_SCREEN_H / 2,
        right - VIDEO_SCREEN_W / 2,
        bottom - VIDEO_SCREEN_H / 2
    );
}

/*
 * camera_unlock()
 * unlocks the camera, so it will move freely in the level
 */
void camera_unlock()
{
    camera.is_locked = false;
}

/*
 * camera_get_position()
 * returns the position of the camera
 */
v2d_t camera_get_position()
{
    return v2d_new(floorf(camera.position.x), floorf(camera.position.y));
}

/*
 * camera_set_position()
 * sets a new position
 */
void camera_set_position(v2d_t position)
{
    camera.position = camera.target = clip_to_boundaries(position);
}

/*
 * camera_is_locked()
 * Is the camera locked?
 */
bool camera_is_locked()
{
    return camera.is_locked;
}

/*
 * camera_clip()
 * Clips a position to the visible playfield
 */
v2d_t camera_clip(v2d_t position)
{
    if(camera.boundaries.enabled) {
        return clip_position(position,
            camera.boundaries.x1 - VIDEO_SCREEN_W / 2,
            camera.boundaries.y1 - VIDEO_SCREEN_H / 2,
            camera.boundaries.x2 + VIDEO_SCREEN_W / 2,
            camera.boundaries.y2 + VIDEO_SCREEN_H / 2
        );
    }
    else
        return position;
}

/*
 * camera_clip_test()
 * Checks if the given position is inside the visible playfield
 */
bool camera_clip_test(v2d_t position)
{
    v2d_t clipped = camera_clip(position);
    return v2d_magnitude(v2d_subtract(position, clipped)) < 1.0f;
}

/* private methods */
void define_boundaries(float x1, float y1, float x2, float y2)
{
    camera.boundaries.x1 = x1;
    camera.boundaries.y1 = y1;
    camera.boundaries.x2 = x2;
    camera.boundaries.y2 = y2;
    sanitize_boundaries();
    enable_boundaries();
}

void reset_boundaries()
{
    camera.boundaries.x1 = 0.0f;
    camera.boundaries.y1 = 0.0f;
    camera.boundaries.x2 = INFINITY;
    camera.boundaries.y2 = INFINITY;
    sanitize_boundaries();
    enable_boundaries();
}

void enable_boundaries()
{
    camera.boundaries.enabled = true;
}

void disable_boundaries()
{
    camera.boundaries.enabled = false;
}

/* ensures x1 <= x2 and y1 <= y2, clipping all coordinates to the playfield */
void sanitize_boundaries()
{
    float min_x = VIDEO_SCREEN_W / 2;
    float max_x = max(min_x, level_size().x - VIDEO_SCREEN_W / 2);
    float min_y = VIDEO_SCREEN_H / 2;
    float max_y = max(min_y, level_size().y - VIDEO_SCREEN_H / 2);

    float x1 = clip(camera.boundaries.x1, min_x, max_x);
    float y1 = clip(camera.boundaries.y1, min_y, max_y);
    float x2 = clip(camera.boundaries.x2, min_x, max_x);
    float y2 = clip(camera.boundaries.y2, min_y, max_y);

    if(x1 > x2)
        x1 = x2 = (x1 + x2) / 2;
    if(y1 > y2)
        y1 = y2 = (y1 + y2) / 2;

    camera.boundaries.x1 = x1;
    camera.boundaries.y1 = y1;
    camera.boundaries.x2 = x2;
    camera.boundaries.y2 = y2;
}

v2d_t clip_to_boundaries(v2d_t position)
{
    if(camera.boundaries.enabled) {
        return clip_position(position,
            camera.boundaries.x1,
            camera.boundaries.y1,
            camera.boundaries.x2,
            camera.boundaries.y2
        );
    }
    else
        return position;
}

v2d_t clip_position(v2d_t position, float x1, float y1, float x2, float y2)
{
    float max_y;

    position.x = clip(position.x, x1, x2);
    position.y = clip(position.y, y1, y2);
    
    max_y = level_height_at(position.x) - VIDEO_SCREEN_H / 2;
    if(position.y > max_y)
        position.y = max_y;

    return position;
}