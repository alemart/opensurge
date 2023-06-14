/*
 * Open Surge Engine
 * particle.c - particle system
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

#include "particle.h"
#include "brick.h"
#include "../scenes/level.h"
#include "../core/video.h"
#include "../core/image.h"
#include "../core/timer.h"
#include "../util/util.h"

/* private stuff ;) */
typedef struct particle_t particle_t;
typedef struct particle_list_t particle_list_t;

/* particle */
struct particle_t {

    /* source image & source rect */
    const image_t* source_image;
    int source_x;
    int source_y;
    int width;
    int height;

    /* particle position */
    v2d_t position;

    /* particle speed */
    v2d_t speed;

};

/* list of particles */
struct particle_list_t {

    particle_t* data;
    particle_list_t* next;

};

static particle_list_t* particle_list = NULL;



/*
 * particle_init()
 * initializes the particle system
 */
void particle_init()
{
    particle_list = NULL;
}

/*
 * particle_release()
 * releases the particle system
 */
void particle_release()
{
    particle_list_t *it, *next;
    particle_t* particle;

    for(it = particle_list; it; it = next) {
        particle = it->data;
        next = it->next;

        free(particle);
        free(it);
    }

    particle_list = NULL;
}

/*
 * particle_add()
 * adds a new particle to the particle system
 */
void particle_add(const struct image_t* source_image, int source_x, int source_y, int width, int height, v2d_t position, v2d_t speed)
{
    particle_t* particle = mallocx(sizeof *particle);
    particle->source_image = source_image;
    particle->source_x = source_x;
    particle->source_y = source_y;
    particle->width = width;
    particle->height = height;
    particle->position = position;
    particle->speed = speed;

    particle_list_t* node = mallocx(sizeof *node);
    node->data = particle;
    node->next = particle_list;
    particle_list = node;
}

/*
 * particle_update()
 * Updates all the particles
 */
void particle_update()
{
    particle_list_t *it, *prev, *next;
    particle_t* particle;
    float dt = timer_get_delta();
    float grv = level_gravity();

    for(it = particle_list, prev = NULL; it; it = next) {
        particle = it->data;
        next = it->next;

        if(level_inside_screen(particle->position.x, particle->position.y, particle->width, particle->height)) {

            /* update this particle */
            particle->speed.y += grv * dt;
            particle->position.x += particle->speed.x * dt;
            particle->position.y += particle->speed.y * dt;
            prev = it;

        }
        else {

            /* remove this particle */
            if(prev)
                prev->next = next;
            else
                particle_list = next;

            free(particle);
            free(it);

        }
    }
}

/*
 * particle_render()
 * Renders the particles
 */
void particle_render(v2d_t camera_position)
{
    v2d_t topleft = v2d_subtract(camera_position, v2d_multiply(video_get_screen_size(), 0.5f));

    /* FIXME: this is assuming that all particles belong to the same bitmap.
       This is not necessarily true, though it generally is if all particles came
       from bricks having the same brickset image.

       Holding the bitmap isn't necessarily "incorrect" if there are multiple images.
       Even though Allegro can handle this, it isn't optimal. */
    image_hold_drawing(true);

    for(particle_list_t* it = particle_list; it; it = it->next) {
        particle_t* particle = it->data;
        v2d_t screen_pos = v2d_subtract(particle->position, topleft);

        image_blit(particle->source_image, particle->source_x, particle->source_y, (int)screen_pos.x, (int)screen_pos.y, particle->width, particle->height);
    }

    image_hold_drawing(false);
}

/*
 * particle_is_empty()
 * Checks if the particle system is empty
 */
bool particle_is_empty()
{
    return particle_list == NULL;
}