/*
 * Open Surge Engine
 * actor.c - actor module
 * Copyright (C) 2008-2012  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <limits.h>
#include <math.h>
#include "actor.h"
#include "brick.h"
#include "../core/global.h"
#include "../core/input.h"
#include "../core/util.h"
#include "../core/logfile.h"
#include "../core/video.h"
#include "../core/timer.h"


/* constants */
#define MAGIC_DIFF              -2  /* platform movement & collision detectors magic */
#define SIDE_CORNERS_HEIGHT     0.5 /* height of the left/right sensors */


/* private data */
static brick_t* brick_at(const brick_list_t *list, v2d_t spot); /* obsolete */

/* private functions */
static void calculate_rotated_boundingbox(const actor_t *act, v2d_t spot[4]);


/* actor functions */



/*
 * actor_create()
 * Creates an actor
 */
actor_t* actor_create()
{
    actor_t *act = mallocx(sizeof *act);

    act->spawn_point = v2d_new(0,0);
    act->position = act->spawn_point;
    act->angle = 0.0f;
    act->speed = v2d_new(0,0);
    act->input = NULL;

    act->animation = NULL;
    act->animation_frame = 0.0f;
    act->animation_speed_factor = 1.0f;
    act->synchronized_animation = FALSE;
    act->mirror = IF_NONE;
    act->visible = TRUE;
    act->alpha = 1.0f;
    act->hot_spot = v2d_new(0,0);
    act->scale = v2d_new(1.0f, 1.0f);

    return act;
}


/*
 * actor_destroy()
 * Destroys an actor
 */
void actor_destroy(actor_t *act)
{
    if(act->input)
        input_destroy(act->input);
    free(act);
}


/*
 * actor_render()
 * Default rendering function
 */
void actor_render(actor_t *act, v2d_t camera_position)
{
    image_t *img;

    if(act->visible && act->animation) {
        /* update animation */
        if(!(act->synchronized_animation) || !(act->animation->repeat)) {
            /* the animation isn't synchronized: every object updates its animation at its own pace */
            act->animation_frame += (act->animation->fps * act->animation_speed_factor) * timer_get_delta();
            if((int)act->animation_frame >= act->animation->frame_count) {
                if(act->animation->repeat)
                    act->animation_frame = (((int)act->animation_frame % act->animation->frame_count) + act->animation->repeat_from) % act->animation->frame_count;
                else
                    act->animation_frame = act->animation->frame_count-1;
            }
        }
        else {
            /* the animation is synchronized: this only makes sense if the animation does repeat */
            act->animation_frame = (act->animation->fps * act->animation_speed_factor) * (0.001f * timer_get_ticks());
            act->animation_frame = (((int)act->animation_frame % act->animation->frame_count) + act->animation->repeat_from) % act->animation->frame_count;
        }

        /* render */
        img = actor_image(act);
        if(fabs(act->angle) > EPSILON)
           image_draw_rotated(img, video_get_backbuffer(), (int)(act->position.x-(camera_position.x-VIDEO_SCREEN_W/2)), (int)(act->position.y-(camera_position.y-VIDEO_SCREEN_H/2)), (int)act->hot_spot.x, (int)act->hot_spot.y, act->angle, act->mirror);
        else if(fabs(act->alpha - 1.0f) > EPSILON)
           image_draw_trans(img, video_get_backbuffer(), (int)(act->position.x-act->hot_spot.x-(camera_position.x-VIDEO_SCREEN_W/2)), (int)(act->position.y-act->hot_spot.y-(camera_position.y-VIDEO_SCREEN_H/2)), act->alpha, act->mirror);
        else if(fabs(act->scale.x - 1.0f) > EPSILON || fabs(act->scale.y - 1.0f) > EPSILON)
           image_draw_scaled(img, video_get_backbuffer(), (int)(act->position.x-act->hot_spot.x*act->scale.x-(camera_position.x-VIDEO_SCREEN_W/2)), (int)(act->position.y-act->hot_spot.y*act->scale.y-(camera_position.y-VIDEO_SCREEN_H/2)), act->scale, act->mirror);
        else
           image_draw(img, video_get_backbuffer(), (int)(act->position.x-act->hot_spot.x-(camera_position.x-VIDEO_SCREEN_W/2)), (int)(act->position.y-act->hot_spot.y-(camera_position.y-VIDEO_SCREEN_H/2)), act->mirror);
    }
}



/*
 * actor_render_repeat_xy()
 * Rendering / repeat xy
 */
void actor_render_repeat_xy(actor_t *act, v2d_t camera_position, int repeat_x, int repeat_y)
{
    int i, j, w, h;
    image_t *img = actor_image(act);
    v2d_t final_pos;

    final_pos.x = (int)act->position.x%(repeat_x?image_width(img):INT_MAX) - act->hot_spot.x-(camera_position.x-VIDEO_SCREEN_W/2) - (repeat_x?image_width(img):0);
    final_pos.y = (int)act->position.y%(repeat_y?image_height(img):INT_MAX) - act->hot_spot.y-(camera_position.y-VIDEO_SCREEN_H/2) - (repeat_y?image_height(img):0);

    if(act->visible && act->animation) {
        /* update animation */
        act->animation_frame += (act->animation->fps * act->animation_speed_factor) * timer_get_delta();
        if((int)act->animation_frame >= act->animation->frame_count) {
            if(act->animation->repeat)
                act->animation_frame = (((int)act->animation_frame % act->animation->frame_count) + act->animation->repeat_from) % act->animation->frame_count;
            else
                act->animation_frame = act->animation->frame_count-1;
        }

        /* render */
        w = repeat_x ? (VIDEO_SCREEN_W/image_width(img) + 3) : 1;
        h = repeat_y ? (VIDEO_SCREEN_H/image_height(img) + 3) : 1;
        for(i=0; i<w; i++) {
            for(j=0; j<h; j++)
                image_draw(img, video_get_backbuffer(), (int)final_pos.x + i*image_width(img), (int)final_pos.y + j*image_height(img), act->mirror);
        }
    }
}


/*
 * actor_collision()
 * Check collisions
 */
int actor_collision(const actor_t *a, const actor_t *b)
{
    int j, right = 0;
    v2d_t corner[2][4];
    corner[0][0] = v2d_subtract(a->position, v2d_rotate(a->hot_spot, -a->angle)); /* a's topleft */
    corner[0][1] = v2d_add( corner[0][0] , v2d_rotate(v2d_new(image_width(actor_image(a)), 0), -a->angle) ); /* a's topright */
    corner[0][2] = v2d_add( corner[0][0] , v2d_rotate(v2d_new(image_width(actor_image(a)), image_height(actor_image(a))), -a->angle) ); /* a's bottomright */
    corner[0][3] = v2d_add( corner[0][0] , v2d_rotate(v2d_new(0, image_height(actor_image(a))), -a->angle) ); /* a's bottomleft */
    corner[1][0] = v2d_subtract(b->position, v2d_rotate(b->hot_spot, -b->angle)); /* b's topleft */
    corner[1][1] = v2d_add( corner[1][0] , v2d_rotate(v2d_new(image_width(actor_image(b)), 0), -b->angle) ); /* b's topright */
    corner[1][2] = v2d_add( corner[1][0] , v2d_rotate(v2d_new(image_width(actor_image(b)), image_height(actor_image(b))), -b->angle) ); /* b's bottomright */
    corner[1][3] = v2d_add( corner[1][0] , v2d_rotate(v2d_new(0, image_height(actor_image(b))), -b->angle) ); /* b's bottomleft */
    right += fabs(a->angle)<EPSILON||fabs(a->angle-PI/2)<EPSILON||fabs(a->angle-PI)<EPSILON||fabs(a->angle-3*PI/2)<EPSILON;
    right += fabs(b->angle)<EPSILON||fabs(b->angle-PI/2)<EPSILON||fabs(b->angle-PI)<EPSILON||fabs(b->angle-3*PI/2)<EPSILON;

    if(right) {
        float r[2][4];
        for(j=0; j<2; j++) {
            r[j][0] = min(corner[j][0].x, corner[j][1].x);
            r[j][1] = min(corner[j][0].y, corner[j][1].y);
            r[j][2] = max(corner[j][2].x, corner[j][3].x);
            r[j][3] = max(corner[j][2].y, corner[j][3].y);
            if(r[j][0] > r[j][2]) swap(r[j][0], r[j][2]);
            if(r[j][1] > r[j][3]) swap(r[j][1], r[j][3]);
        }
        return bounding_box(r[0],r[1]);
    }
    else {
        v2d_t center[2];
        float radius[2] = { max(image_width(actor_image(a)),image_height(actor_image(a))) , max(image_width(actor_image(b)),image_height(actor_image(b))) };
        for(j=0; j<2; j++)
            center[j] = v2d_multiply(v2d_add(corner[j][0], corner[j][2]), 0.5);
        return circular_collision(center[0], radius[0], center[1], radius[1]);
    }
}


/*
 * actor_orientedbox_collision()
 * Is a colliding with b? (oriented bounding box detection)
 */
int actor_orientedbox_collision(const actor_t *a, const actor_t *b)
{
    v2d_t a_pos, b_pos;
    v2d_t a_size, b_size;
    v2d_t a_spot[4], b_spot[4]; /* rotated spots */

    calculate_rotated_boundingbox(a, a_spot);
    calculate_rotated_boundingbox(b, b_spot);

    a_pos.x = min(a_spot[0].x, min(a_spot[1].x, min(a_spot[2].x, a_spot[3].x)));
    a_pos.y = min(a_spot[0].y, min(a_spot[1].y, min(a_spot[2].y, a_spot[3].y)));
    b_pos.x = min(b_spot[0].x, min(b_spot[1].x, min(b_spot[2].x, b_spot[3].x)));
    b_pos.y = min(b_spot[0].y, min(b_spot[1].y, min(b_spot[2].y, b_spot[3].y)));

    a_size.x = max(a_spot[0].x, max(a_spot[1].x, max(a_spot[2].x, a_spot[3].x))) - a_pos.x;
    a_size.y = max(a_spot[0].y, max(a_spot[1].y, max(a_spot[2].y, a_spot[3].y))) - a_pos.y;
    b_size.x = max(b_spot[0].x, max(b_spot[1].x, max(b_spot[2].x, b_spot[3].x))) - b_pos.x;
    b_size.y = max(b_spot[0].y, max(b_spot[1].y, max(b_spot[2].y, b_spot[3].y))) - b_pos.y;

    if(a_pos.x + a_size.x >= b_pos.x && a_pos.x <= b_pos.x + b_size.x) {
        if(a_pos.y + a_size.y >= b_pos.y && a_pos.y <= b_pos.y + b_size.y)
            return TRUE;
    }

    return FALSE;
}


/*
 * actor_pixelperfect_collision()
 * Is a colliding with b? (pixel perfect detection)
 */
int actor_pixelperfect_collision(const actor_t *a, const actor_t *b)
{
    if(fabs(a->angle) < EPSILON && fabs(b->angle) < EPSILON && a->mirror == IF_NONE && b->mirror == IF_NONE) {
        if(actor_collision(a, b)) {
            int x1, y1, x2, y2;

            x1 = (int)(a->position.x - a->hot_spot.x);
            y1 = (int)(a->position.y - a->hot_spot.y);
            x2 = (int)(b->position.x - b->hot_spot.x);
            y2 = (int)(b->position.y - b->hot_spot.y);

            return image_pixelperfect_collision(actor_image(a), actor_image(b), x1, y1, x2, y2);
        }
        else
            return FALSE;
    }
    else {
        if(actor_orientedbox_collision(a, b)) {
            image_t *image_a, *image_b;
            v2d_t size_a, size_b, pos_a, pos_b;
            v2d_t a_spot[4], b_spot[4]; /* rotated spots */
            v2d_t ac, bc; /* rotation spot */
            int collided;

            calculate_rotated_boundingbox(a, a_spot);
            calculate_rotated_boundingbox(b, b_spot);

            pos_a.x = min(a_spot[0].x, min(a_spot[1].x, min(a_spot[2].x, a_spot[3].x)));
            pos_a.y = min(a_spot[0].y, min(a_spot[1].y, min(a_spot[2].y, a_spot[3].y)));
            pos_b.x = min(b_spot[0].x, min(b_spot[1].x, min(b_spot[2].x, b_spot[3].x)));
            pos_b.y = min(b_spot[0].y, min(b_spot[1].y, min(b_spot[2].y, b_spot[3].y)));

            size_a.x = max(a_spot[0].x, max(a_spot[1].x, max(a_spot[2].x, a_spot[3].x))) - pos_a.x;
            size_a.y = max(a_spot[0].y, max(a_spot[1].y, max(a_spot[2].y, a_spot[3].y))) - pos_a.y;
            size_b.x = max(b_spot[0].x, max(b_spot[1].x, max(b_spot[2].x, b_spot[3].x))) - pos_b.x;
            size_b.y = max(b_spot[0].y, max(b_spot[1].y, max(b_spot[2].y, b_spot[3].y))) - pos_b.y;

            ac = v2d_add(v2d_subtract(a_spot[0], pos_a), v2d_rotate(a->hot_spot, -a->angle));
            bc = v2d_add(v2d_subtract(b_spot[0], pos_b), v2d_rotate(b->hot_spot, -b->angle));

            image_a = image_create(size_a.x, size_a.y);
            image_b = image_create(size_b.x, size_b.y);
            image_clear(image_a, video_get_maskcolor());
            image_clear(image_b, video_get_maskcolor());

            image_draw_rotated(actor_image(a), image_a, ac.x, ac.y, (int)a->hot_spot.x, (int)a->hot_spot.y, a->angle, a->mirror);
            image_draw_rotated(actor_image(b), image_b, bc.x, bc.y, (int)b->hot_spot.x, (int)b->hot_spot.y, b->angle, b->mirror);

            collided = image_pixelperfect_collision(image_a, image_b, pos_a.x, pos_a.y, pos_b.x, pos_b.y);

            image_destroy(image_a);
            image_destroy(image_b);
            return collided;
        }
        else
            return FALSE;
    }
}


/*
 * actor_brick_collision()
 * Actor collided with a brick?
 */
int actor_brick_collision(actor_t *act, brick_t *brk)
{
    v2d_t topleft = v2d_subtract(act->position, v2d_rotate(act->hot_spot, act->angle));
    v2d_t bottomright = v2d_add( topleft , v2d_rotate(v2d_new(image_width(actor_image(act)), image_height(actor_image(act))), act->angle) );
    float a[4] = { topleft.x , topleft.y , bottomright.x , bottomright.y };
    float b[4] = { (float)brk->x , (float)brk->y , (float)(brk->x+image_width(brk->brick_ref->image)) , (float)(brk->y+image_height(brk->brick_ref->image)) };

    return bounding_box(a,b);
}


/*
 * actor_change_animation()
 * Changes the animation of an actor
 */
void actor_change_animation(actor_t *act, animation_t *anim)
{
    if(act->animation != anim) {
        act->animation = anim;
        act->hot_spot = anim->hot_spot;
        act->animation_frame = 0;
        act->animation_speed_factor = 1.0;
    }
}



/*
 * actor_change_animation_frame()
 * Changes the animation frame
 */
void actor_change_animation_frame(actor_t *act, int frame)
{
    act->animation_frame = clip(frame, 0, act->animation->frame_count - 1);
}



/*
 * actor_change_animation_speed_factor()
 * Changes the speed factor of the current animation
 * The default factor is 1.0 (i.e., 100% of the original
 * animation speed)
 */
void actor_change_animation_speed_factor(actor_t *act, float factor)
{
    act->animation_speed_factor = max(0.0, factor);
}



/*
 * actor_animation_finished()
 * Returns true if the current animation has finished
 */
int actor_animation_finished(actor_t *act)
{
    float frame = act->animation_frame + (act->animation->fps * act->animation_speed_factor) * timer_get_delta();
    return (!act->animation->repeat && (int)frame >= act->animation->frame_count);
}



/*
 * actor_synchronize_animation()
 * should I use a shared animation frame?
 */
void actor_synchronize_animation(actor_t *act, int sync)
{
    act->synchronized_animation = sync;
}


/*
 * actor_image()
 * Returns the current image of the
 * animation of this actor
 */
image_t* actor_image(const actor_t *act)
{
    return sprite_get_image(act->animation, (int)act->animation_frame);
}






/*
 * actor_sensors()
 * Get obstacle bricks around the actor
 */
void actor_sensors(actor_t *act, brick_list_t *brick_list, brick_t **up, brick_t **upright, brick_t **right, brick_t **downright, brick_t **down, brick_t **downleft, brick_t **left, brick_t **upleft)
{
    float diff = MAGIC_DIFF;
    int frame_width = image_width(actor_image(act));
    int frame_height = image_height(actor_image(act));

    v2d_t feet       = v2d_add(v2d_subtract(act->position, act->hot_spot), v2d_new(frame_width/2, frame_height));
    v2d_t vup        = v2d_add ( feet , v2d_rotate( v2d_new(0, -frame_height+diff), -act->angle) );
    v2d_t vdown      = v2d_add ( feet , v2d_rotate( v2d_new(0, -diff), -act->angle) ); 
    v2d_t vleft      = v2d_add ( feet , v2d_rotate( v2d_new(-frame_width/2+diff, -frame_height*SIDE_CORNERS_HEIGHT), -act->angle) );
    v2d_t vright     = v2d_add ( feet , v2d_rotate( v2d_new(frame_width/2-diff, -frame_height*SIDE_CORNERS_HEIGHT), -act->angle) );
    v2d_t vupleft    = v2d_add ( feet , v2d_rotate( v2d_new(-frame_width/2+diff, -frame_height+diff), -act->angle) );
    v2d_t vupright   = v2d_add ( feet , v2d_rotate( v2d_new(frame_width/2-diff, -frame_height+diff), -act->angle) );
    v2d_t vdownleft  = v2d_add ( feet , v2d_rotate( v2d_new(-frame_width/2+diff, -diff), -act->angle) );
    v2d_t vdownright = v2d_add ( feet , v2d_rotate( v2d_new(frame_width/2-diff, -diff), -act->angle) );

    actor_sensors_ex(act, vup, vupright, vright, vdownright, vdown, vdownleft, vleft, vupleft, brick_list, up, upright, right, downright, down, downleft, left, upleft);
}




/*
 * actor_sensors_ex()
 * Like actor_sensors(), but this procedure allows to specify the
 * collision detectors positions'
 */
void actor_sensors_ex(actor_t *act, v2d_t vup, v2d_t vupright, v2d_t vright, v2d_t vdownright, v2d_t vdown, v2d_t vdownleft, v2d_t vleft, v2d_t vupleft, brick_list_t *brick_list, brick_t **up, brick_t **upright, brick_t **right, brick_t **downright, brick_t **down, brick_t **downleft, brick_t **left, brick_t **upleft)
{
    float diff = MAGIC_DIFF;
    brick_t **cloud_off[5] = { up, upright, right, left, upleft };
    int i;

    if(up) *up = brick_at(brick_list, vup);
    if(down) *down = brick_at(brick_list, vdown);
    if(left) *left = brick_at(brick_list, vleft);
    if(right) *right = brick_at(brick_list, vright);
    if(upleft) *upleft = brick_at(brick_list, vupleft);
    if(upright) *upright = brick_at(brick_list, vupright);
    if(downleft) *downleft = brick_at(brick_list, vdownleft);
    if(downright) *downright = brick_at(brick_list, vdownright);


    /* handle clouds */

    /* bricks: laterals and top */
    for(i=0; i<5; i++) {
        /* forget bricks */
        brick_t **brk = cloud_off[i];
        if(brk && *brk && (*brk)->brick_ref && (*brk)->brick_ref->property == BRK_CLOUD)
            *brk = NULL;
    }

    /* bricks: down, downleft, downright */
    if(down && *down && (*down)->brick_ref->property == BRK_CLOUD) {
        float offset = min(15, image_height((*down)->brick_ref->image)/3);
        if(!(act->speed.y >= 0 && act->position.y < ((*down)->y+diff+1)+offset)) {
            /* forget bricks */
            if(downleft && *downleft == *down)
                *downleft = NULL;
            if(downright && *downright == *down)
                *downright = NULL;
            *down = NULL;
        }
    }
}



/*
 * actor_brick_at()
 * Gets a brick at a certain offset (may return NULL)
 */
const brick_t* actor_brick_at(actor_t *act, const brick_list_t *brick_list, v2d_t offset)
{
    return brick_at(brick_list, v2d_add(act->position, offset));
}






/* private stuff */

/* brick_at(): given a list of bricks, returns
 * one that collides with the given spot
 * PS: this code ignores the bricks that are
 * not obstacles */
/* NOTE: this is old (deprecated) code -- see obstaclemap.c */
static brick_t* brick_at(const brick_list_t *list, v2d_t spot)
{
    const brick_list_t *p;
    brick_t *ret = NULL;
    v2d_t offset;
    float br[4];

    /* main algorithm */
    for(p=list; p; p=p->next) {

        /* ignore passable bricks */
        if(p->data->brick_ref->property == BRK_NONE)
            continue;

        /* I don't want clouds. */
        if(p->data->brick_ref->property == BRK_CLOUD && (ret && ret->brick_ref->property == BRK_OBSTACLE))
            continue;

        /* I don't want moving platforms */
        if(p->data->brick_ref->behavior == BRB_CIRCULAR && (ret && ret->brick_ref->behavior != BRB_CIRCULAR) && p->data->y >= ret->y)
            continue;

        /* here's something I want... */
        br[0] = (float)p->data->x;
        br[1] = (float)p->data->y;
        br[2] = (float)(p->data->x + image_width(p->data->brick_ref->image));
        br[3] = (float)(p->data->y + image_height(p->data->brick_ref->image));
        offset = v2d_subtract(spot, v2d_new(p->data->x, p->data->y));

        if((spot.x >= br[0] && spot.y >= br[1] && spot.x < br[2] && spot.y < br[3]) && image_getpixel(brick_image(p->data), (int)offset.x, (int)offset.y) != video_get_maskcolor()) {
            if(p->data->brick_ref->behavior != BRB_CIRCULAR && (ret && ret->brick_ref->behavior == BRB_CIRCULAR) && p->data->y <= ret->y) {
                ret = p->data; /* No moving platforms. Let's grab a regular platform instead. */
            }
            else if(p->data->brick_ref->property == BRK_OBSTACLE && (ret && ret->brick_ref->property == BRK_CLOUD)) {
                ret = p->data; /* No clouds. Let's grab an obstacle instead. */
            }
            else if(p->data->brick_ref->property == BRK_CLOUD && (ret && ret->brick_ref->property == BRK_CLOUD)) {
                /* oh no, two conflicting clouds! */
                if(p->data->y > ret->y)
                    ret = p->data;
            }
            else
                ret = p->data;
        }
    }

    return ret;
}


/*
 * calculate_rotated_boundingbox()
 * Calculates the rotated bounding box of a given actor
 */
void calculate_rotated_boundingbox(const actor_t *act, v2d_t spot[4])
{
    float w, h, angle;
    v2d_t a, b, c, d, hs;
    v2d_t pos;

    angle = -act->angle;
    w = image_width(actor_image(act));
    h = image_height(actor_image(act));
    hs = act->hot_spot;
    pos = act->position;

    a = v2d_subtract(v2d_new(0, 0), hs);
    b = v2d_subtract(v2d_new(w, 0), hs);
    c = v2d_subtract(v2d_new(w, h), hs);
    d = v2d_subtract(v2d_new(0, h), hs);

    spot[0] = v2d_add(pos, v2d_rotate(a, angle));
    spot[1] = v2d_add(pos, v2d_rotate(b, angle));
    spot[2] = v2d_add(pos, v2d_rotate(c, angle));
    spot[3] = v2d_add(pos, v2d_rotate(d, angle));
}

