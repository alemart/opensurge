/*
 * Open Surge Engine
 * particle.c - particle system
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

#include "particle.h"
#include "brick.h"
#include "../scenes/level.h"
#include "../core/video.h"
#include "../core/image.h"
#include "../core/util.h"
#include "../core/timer.h"

/* private stuff ;) */
typedef struct {
    image_t *image;
    v2d_t position;
    v2d_t speed;
    int destroy_on_brick;
} particle_t;

typedef struct particle_list_t {
    particle_t *data;
    struct particle_list_t *next;
} particle_list_t;

static particle_list_t *particle_list = NULL;



/* initializes the particle system */
void particle_init()
{
    particle_list = NULL;
}

/* releases the particle system */
void particle_release()
{
    particle_list_t *it, *next;
    particle_t *p;

    for(it=particle_list; it; it=next) {
        p = it->data;
        next = it->next;

        image_destroy(p->image);
        free(p);
        free(it);
    }

    particle_list = NULL;
}

/* adds a new particle to the system. Warning: image will be free'd internally. */
void particle_add(struct image_t *image, v2d_t position, v2d_t speed, int destroy_on_brick)
{
    particle_t *p;
    particle_list_t *node;

    p = mallocx(sizeof *p);
    p->image = image;
    p->position = position;
    p->speed = speed;
    p->destroy_on_brick = destroy_on_brick;

    node = mallocx(sizeof *node);
    node->data = p;
    node->next = particle_list;
    particle_list = node;
}

/* updates all the particles */
void particle_update_all(const struct brick_list_t* brick_list)
{
    float dt = timer_get_delta(), g = level_gravity();
    int got_brick, inside_area;
    particle_list_t *it, *prev = NULL, *next;
    particle_t *p;

    for(it=particle_list; it; it=next) {
        p = it->data;
        next = it->next;
        inside_area = level_inside_screen(p->position.x, p->position.y, p->position.x+image_width(p->image), p->position.y+image_height(p->image));

        /* collided with bricks? */
        got_brick = FALSE;
        if(p->destroy_on_brick && inside_area && p->speed.y > 0) {
            float a[4] = { p->position.x, p->position.y, p->position.x+image_width(p->image), p->position.y+image_height(p->image) };
            const brick_list_t *itb;
            for(itb=brick_list; itb && !got_brick; itb=itb->next) {
                const brick_t *brk = itb->data;
                if(brk->brick_ref->property == BRK_OBSTACLE && brk->brick_ref->angle == 0) {
                    float b[4] = { brk->x, brk->y, brk->x+image_width(brk->brick_ref->image), brk->y+image_height(brk->brick_ref->image) };
                    if(bounding_box(a,b))
                        got_brick = TRUE;
                }
            }
        }

        /* update particle */
        if(!inside_area || got_brick) {
            /* remove this particle */
            if(prev)
                prev->next = next;
            else
                particle_list = next;

            image_destroy(p->image);
            free(p);
            free(it);
        }
        else {
            /* update this particle */
            p->position.x += p->speed.x*dt;
            p->position.y += p->speed.y*dt + 0.5*g*(dt*dt);
            p->speed.y += g*dt;
            prev = it;
        }
    }
}

/* renders the particles */
void particle_render_all(v2d_t camera_position)
{
    particle_list_t *it;
    particle_t *p;
    v2d_t topleft = v2d_new(camera_position.x-VIDEO_SCREEN_W/2, camera_position.y-VIDEO_SCREEN_H/2);

    for(it=particle_list; it; it=it->next) {
        p = it->data;
        image_draw(p->image, video_get_backbuffer(), (int)(p->position.x-topleft.x), (int)(p->position.y-topleft.y), IF_NONE);
    }
}

