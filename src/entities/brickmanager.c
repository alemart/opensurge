/*
 * Open Surge Engine
 * brickmanager.c - brick manager
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

#include <stdint.h>
#include <stdbool.h>
#include "brickmanager.h"
#include "brick.h"
#include "../util/util.h"
#include "../util/darray.h"
#include "../util/iterator.h"

#define FASTHASH_INLINE
#include "../util/fasthash.h"

typedef struct brickrect_t brickrect_t;
typedef struct heightsampler_t heightsampler_t;
typedef struct brickbucket_t brickbucket_t;
typedef struct brickiteratorstate_t brickiteratorstate_t;

/* A rectangle in world space */
struct brickrect_t
{
    /* coordinates are inclusive */
    int top;
    int left;
    int bottom;
    int right;
};

/* A height sampler is used to measure the height of the world in fixed-size intervals */
struct heightsampler_t
{
    /* height of the world at fixed-size intervals */
    DARRAY(int, height_at);

    /* smooth_height_at[j] = smooth_height_at[j-1] if j >= 1 and height_at[j] == 0 (no sampling data)
                             height_at[j]          otherwise */
    DARRAY(int, smooth_height_at);
};

/* A bucket of bricks */
struct brickbucket_t
{
    /* a vector of bricks */
    DARRAY(brick_t*, brick);

    /* a destructor of individual bricks */
    brick_t* (*brick_dtor)(brick_t*);
};

/* Brick Manager */
struct brickmanager_t
{
    /* The Brick Manager is implemented with a spatial hash table mapped to a
       linear map table */

    /* a hash table of brick buckets that are allocated lazily */
    fasthash_t* hashtable;

    /* a special bucket that is included in all queries regardless of the ROI */
    brickbucket_t* awake_bucket;

    /* references to all allocated buckets (for quick access) */
    DARRAY(const brickbucket_t*, bucket_ref);

    /* current region of interest */
    brickrect_t roi;

    /* how many bricks are there? */
    int brick_count;

    /* world size */
    int world_width;
    int world_height;

    /* height sampler */
    heightsampler_t* sampler;
};

/* Iterator state */
struct brickiteratorstate_t
{
    /* a vector of references to non-empty buckets */
    DARRAY(const brickbucket_t*, bucket);

    /* bucket cursor */
    int b;

    /* brick index corresponding to bucket b */
    int i;

    /* a possibly empty bucket of references to awake bricks inside the ROI */
    brickbucket_t* own_bucket;
};

/* Utilities */
#define GRID_SIZE 256 /* width and height of a cell of the spatial hash; this impacts the number of fasthash queries per frame (quadratically), as well as the number of returned bricks */
#define SAMPLER_WIDTH 128 /* width of the fixed-size intervals of the sampler */
#define SAMPLER_MAX_INDEX 16384 /* >= MAX_LEVEL_WIDTH / SAMPLER_WIDTH */

static inline uint64_t position_to_hash(int x, int y);
static inline uint64_t brick2hash(const brick_t* brick);

static brickbucket_t* bucket_ctor(brick_t* (*brick_dtor)(brick_t*));
static brickbucket_t* bucket_dtor(brickbucket_t* bucket);
static void bucket_dtor_adapter(void* bucket);
static inline void bucket_add(brickbucket_t* bucket, brick_t* brick);
static int bucket_wash(brickbucket_t* bucket);
static void bucket_clear(brickbucket_t* bucket);
static inline bool bucket_is_empty(const brickbucket_t* bucket);

static void* brickiteratorstate_copy_ctor(void* data);
static void brickiteratorstate_dtor(void* s);
static void* brickiteratorstate_next(void* s);
static bool brickiteratorstate_has_next(void* s);

static heightsampler_t* sampler_ctor();
static heightsampler_t* sampler_dtor(heightsampler_t* sampler);
static void sampler_clear(heightsampler_t* sampler);
static void sampler_add(heightsampler_t* sampler, const brick_t* brick);
static int sampler_query(heightsampler_t* sampler, int left, int right);

static void update_world_size(brickmanager_t* manager, const brick_t* brick);

static bool is_brick_inside_roi(const brick_t* brick, const brickrect_t* roi);
static void filter_bricks_inside_roi(brickbucket_t* out_bucket, const brickbucket_t* in_bucket, const brickrect_t* roi);
static void filter_non_default_bricks(brickbucket_t* out_bucket, const brickbucket_t* in_bucket);

static brick_t* brick_fake_destroy(brick_t* brick);

static brick_list_t* add_to_list(brick_list_t* list, brick_t* brick);
static brick_list_t* release_list(brick_list_t* list);



/*
 * public API
 */




/*
 * brickmanager_create()
 * Creates a new Brick Manager
 */
brickmanager_t* brickmanager_create()
{
    brickmanager_t* manager = mallocx(sizeof *manager);

    manager->hashtable = fasthash_create(bucket_dtor_adapter, 12);
    manager->awake_bucket = bucket_ctor(brick_destroy);
    darray_init(manager->bucket_ref);
    darray_push(manager->bucket_ref, manager->awake_bucket);
    manager->sampler = sampler_ctor();

    manager->roi = (brickrect_t){ 0, 0, 0, 0 };
    manager->brick_count = 0;
    manager->world_width = 1;
    manager->world_height = 1;

    return manager;
}

/*
 * brickmanager_destroy()
 * Destroys an existing Brick Manager
 */
brickmanager_t* brickmanager_destroy(brickmanager_t* manager)
{
    sampler_dtor(manager->sampler);
    darray_release(manager->bucket_ref); /* a vector of references only */
    bucket_dtor(manager->awake_bucket);
    fasthash_destroy(manager->hashtable);

    free(manager);
    return NULL;
}

/*
 * brickmanager_add_brick()
 * Adds an existing brick to the Brick Manager
 */
void brickmanager_add_brick(brickmanager_t* manager, struct brick_t* brick)
{
    brickbucket_t* bucket = NULL;
    bool is_moving_brick = brick_has_movement_path(brick);

    /* select a bucket */
    if(!is_moving_brick) {

        /* find the appropriate bucket for the brick */
        uint64_t key = brick2hash(brick);
        bucket = fasthash_get(manager->hashtable, key);

        /* lazily allocate a new bucket if one doesn't exist */
        if(bucket == NULL) {
            bucket = bucket_ctor(brick_destroy);
            fasthash_put(manager->hashtable, key, bucket);
            darray_push(manager->bucket_ref, bucket);
        }

    }
    else {

        /* we add moving bricks to the awake bucket */
        bucket = manager->awake_bucket;

    }

    /* add the brick to the bucket */
    bucket_add(bucket, brick);

    /* increment the brick count */
    manager->brick_count++;

    /* update the size of the world */
    update_world_size(manager, brick);

    /* update the height sampler */
    sampler_add(manager->sampler, brick);
}

/*
 * brickmanager_remove_all_bricks()
 * Removes all bricks
 */
void brickmanager_remove_all_bricks(brickmanager_t* manager)
{
    /* clear all buckets */
    for(int i = 0; i < darray_length(manager->bucket_ref); i++)
        bucket_clear((brickbucket_t*)manager->bucket_ref[i]);

    /* reset the sampler */
    sampler_clear(manager->sampler);

    /* reset stats */
    manager->brick_count = 0;
    manager->world_width = 1;
    manager->world_height = 1;
}

/*
 * brickmanager_update()
 * Updates the Brick Manager
 */
void brickmanager_update(brickmanager_t* manager)
{
    /* remove dead bricks inside (any bucket that intersects with) the ROI */
    int cnt = 0; /* we'll count the number of removed bricks */
    int left = manager->roi.left;
    int top = manager->roi.top;
    int right = manager->roi.right;
    int bottom = manager->roi.bottom;

    right += GRID_SIZE - 1;
    bottom += GRID_SIZE - 1;

    for(int y = top; y <= bottom; y += GRID_SIZE) {
        for(int x = left; x <= right; x += GRID_SIZE) {
            uint64_t key = position_to_hash(x, y);
            brickbucket_t* bucket = fasthash_get(manager->hashtable, key);

            /* wash the bucket if it exists */
            if(bucket != NULL)
                cnt += bucket_wash(bucket);
        }
    }

    /* remove dead bricks stored in the awake bucket */
    cnt += bucket_wash(manager->awake_bucket);

    /* update the brick count */
    manager->brick_count -= cnt;

    /* we don't update the sampler nor the world size: why bother?
       doesn't matter much, since dead bricks are very few with special behavior
       we may also remove bricks using the level editor, but we can just recalculate instead */
}

/*
 * brickmanager_number_of_bricks()
 * How many bricks are there in world space?
 */
int brickmanager_number_of_bricks(const brickmanager_t* manager)
{
    return manager->brick_count;
}

/*
 * brickmanager_world_size()
 * Get the world size, in pixels
 */
void brickmanager_world_size(const brickmanager_t* manager, int* world_width, int* world_height)
{
    if(manager->brick_count > 0) {
        /* if there are bricks, we know the actual size of the world */
        *world_width = manager->world_width;
        *world_height = manager->world_height;
    }
    else {
        /* if the world is empty of bricks, we consider it to be very large
           the camera may be clipped to a tiny area if we don't */
        *world_width = LARGE_INT;
        *world_height = LARGE_INT;
    }
}

/*
 * brickmanager_world_height_at_interval()
 * Get the height of the world at the given interval
 * Coordinates are inclusive
 */
int brickmanager_world_height_at_interval(const brickmanager_t* manager, int left_xpos, int right_xpos)
{
    /* no sampling data? */
    if(manager->brick_count == 0)
        return LARGE_INT;

    /* return sampling data */
    return sampler_query(manager->sampler, left_xpos, right_xpos);
}

/*
 * brickmanager_recalculate_world_size()
 * Recalculate world size
 */
void brickmanager_recalculate_world_size(brickmanager_t* manager)
{
    /* reset the world size */
    manager->world_width = 1;
    manager->world_height = 1;

    /* reset the sampler */
    sampler_clear(manager->sampler);

    /* iterate over all bricks to recalculate */
    for(int b = 0; b < darray_length(manager->bucket_ref); b++) {
        const brickbucket_t* bucket = manager->bucket_ref[b];

        for(int i = 0; i < darray_length(bucket->brick); i++) {
            const brick_t* brick = bucket->brick[i];

            update_world_size(manager, brick);
            sampler_add(manager->sampler, brick);
        }
    }
}

/*
 * brickmanager_set_roi()
 * Sets the current Region Of Interest (ROI) in world space.
 * Coordinates are inclusive.
 */
void brickmanager_set_roi(brickmanager_t* manager, rect_t roi)
{
    int x = roi.x;
    int y = roi.y;
    int width = roi.width;
    int height = roi.height;
    int world_width = manager->world_width;
    int world_height = manager->world_height;

    /*
    
    clip values:

    0 <= x <= world_width - 1
    0 <= y <= world_height - 1
    1 <= width <= world_width - x
    1 <= height <= world_height - y

    a unrealistically large ROI could cause unnecessary slowdowns,
    so we clip it.
    
    */

    if(x < 0)
        x = 0;
    if(x > world_width - 1)
        x = world_width - 1;

    if(y < 0)
        y = 0;
    if(y > world_height - 1)
        y = world_height - 1;

    if(width < 1)
        width = 1;
    if(x + width > world_width)
        width = world_width - x;

    if(height < 1)
        height = 1;
    if(y + height > world_height)
        height = world_height - y;

    /* update the ROI */
    manager->roi.left = x;
    manager->roi.top = y;
    manager->roi.right = x + width - 1;
    manager->roi.bottom = y + height - 1;
}

/*
 * brickmanager_retrieve_active_bricks()
 * Efficiently retrieve bricks inside the current Region Of Interest (ROI)
 */
iterator_t* brickmanager_retrieve_active_bricks(const brickmanager_t* manager)
{
    /* create a new iterator state */
    brickiteratorstate_t state = { .b = 0, .i = 0 };
    state.own_bucket = bucket_ctor(brick_fake_destroy); /* a bucket of references only */
    darray_init(state.bucket);

    /* get the ROI */
    const brickrect_t* roi = &(manager->roi);

    /* for each bucket inside the ROI */
    int left = roi->left;
    int top = roi->top;
    int right = roi->right;
    int bottom = roi->bottom;

    right += GRID_SIZE - 1;
    bottom += GRID_SIZE - 1;

    for(int y = top; y <= bottom; y += GRID_SIZE) {
        for(int x = left; x <= right; x += GRID_SIZE) {
            uint64_t key = position_to_hash(x, y);
            const brickbucket_t* bucket = fasthash_get(manager->hashtable, key);

            /* add the bucket if it exists and if it's not empty */
            if(bucket != NULL && !bucket_is_empty(bucket))
                darray_push(state.bucket, bucket);
        }
    }

    /* individually filter the awake bricks inside the ROI */
    filter_bricks_inside_roi(state.own_bucket, manager->awake_bucket, roi);
    if(!bucket_is_empty(state.own_bucket))
        darray_push(state.bucket, state.own_bucket);

    /* return a new iterator */
    return iterator_create(
        &state,
        brickiteratorstate_copy_ctor,
        brickiteratorstate_dtor,
        brickiteratorstate_next,
        brickiteratorstate_has_next
    );
}

/*
 * brickmanager_retrieve_active_moving_bricks()
 * Efficiently retrieve moving bricks inside the current Region Of Interest (ROI)
 */
iterator_t* brickmanager_retrieve_active_moving_bricks(const brickmanager_t* manager)
{
    /* create a new iterator state */
    brickiteratorstate_t state = { .b = 0, .i = 0 };
    state.own_bucket = bucket_ctor(brick_fake_destroy); /* a bucket of references only */
    darray_init(state.bucket);

    /* get the ROI */
    const brickrect_t* roi = &(manager->roi);

    /* for each bucket inside the ROI */
    int left = roi->left;
    int top = roi->top;
    int right = roi->right;
    int bottom = roi->bottom;

    right += GRID_SIZE - 1;
    bottom += GRID_SIZE - 1;

    for(int y = top; y <= bottom; y += GRID_SIZE) {
        for(int x = left; x <= right; x += GRID_SIZE) {
            uint64_t key = position_to_hash(x, y);
            const brickbucket_t* bucket = fasthash_get(manager->hashtable, key);

            /* we must consider bricks with non-default behavior as "moving" */
            /* we add the bucket if it exists and if it's not empty */
            if(bucket != NULL && !bucket_is_empty(bucket))
                filter_non_default_bricks(state.own_bucket, bucket);
        }
    }

    /* individually filter the awake bricks inside the ROI */
    filter_bricks_inside_roi(state.own_bucket, manager->awake_bucket, roi);

    /* add own_bucket if it's not empty */
    if(!bucket_is_empty(state.own_bucket))
        darray_push(state.bucket, state.own_bucket);

    /* return a new iterator */
    return iterator_create(
        &state,
        brickiteratorstate_copy_ctor,
        brickiteratorstate_dtor,
        brickiteratorstate_next,
        brickiteratorstate_has_next
    );
}

/*
 * brickmanager_retrieve_all_bricks()
 * Retrieves all bricks
 */
iterator_t* brickmanager_retrieve_all_bricks(const brickmanager_t* manager)
{
    /* create a new iterator state */
    brickiteratorstate_t state = { .b = 0, .i = 0 };
    state.own_bucket = bucket_ctor(brick_fake_destroy); /* a bucket of references only */
    darray_init(state.bucket);

    /* we'll iterate over all non-empty buckets */
    for(int i = 0; i < darray_length(manager->bucket_ref); i++) {
        if(!bucket_is_empty(manager->bucket_ref[i]))
            darray_push(state.bucket, manager->bucket_ref[i]);
    }

    /* return a new iterator */
    return iterator_create(
        &state,
        brickiteratorstate_copy_ctor,
        brickiteratorstate_dtor,
        brickiteratorstate_next,
        brickiteratorstate_has_next
    );
}

/*
 * brickmanager_retrieve_all_bricks_as_list()
 * Retrieves all bricks as a brick list
 */
brick_list_t* brickmanager_retrieve_all_bricks_as_list(const brickmanager_t* manager)
{
    iterator_t* it = brickmanager_retrieve_all_bricks(manager);
    brick_list_t* list = NULL;

    while(iterator_has_next(it)) {
        brick_t* brick = iterator_next(it);
        list = add_to_list(list, brick);
    }

    iterator_destroy(it);
    return list;
}

/*
 * brickmanager_retrieve_active_bricks_as_list()
 * Retrieves bricks inside the ROI as a brick list
 */
brick_list_t* brickmanager_retrieve_active_bricks_as_list(const brickmanager_t* manager)
{
    iterator_t* it = brickmanager_retrieve_active_bricks(manager);
    brick_list_t* list = NULL;

    while(iterator_has_next(it)) {
        brick_t* brick = iterator_next(it);
        list = add_to_list(list, brick);
    }

    iterator_destroy(it);
    return list;
}

/*
 * brickmanager_release_list()
 * Releases a brick list
 */
brick_list_t* brickmanager_release_list(brick_list_t* list)
{
    return release_list(list);
}





/*
 * private stuff
 */


/* hashing utilities */

uint64_t position_to_hash(int x, int y)
{
    if(x < 0)
        x = 0;

    if(y < 0)
        y = 0;

    x /= GRID_SIZE;
    y /= GRID_SIZE;

    return (((uint64_t)x) << 32) | ((uint64_t)y);
}

uint64_t brick2hash(const brick_t* brick)
{
    /* the spawn point does not change !!!
       the position may change and we do not keep track of position changes */
    v2d_t topleft = brick_spawnpoint(brick);
    v2d_t size = brick_size(brick);

    int center_x = topleft.x + size.x * 0.5f;
    int center_y = topleft.y + size.y * 0.5f;

    return position_to_hash(center_x, center_y);
}




/* buckets */

brickbucket_t* bucket_ctor(brick_t* (*brick_dtor)(brick_t*))
{
    brickbucket_t* bucket = mallocx(sizeof *bucket);

    darray_init(bucket->brick);
    bucket->brick_dtor = brick_dtor;

    return bucket;
}

brickbucket_t* bucket_dtor(brickbucket_t* bucket)
{
    /* release all bricks of the bucket */
    for(int i = darray_length(bucket->brick) - 1; i >= 0; i--)
        bucket->brick_dtor(bucket->brick[i]);

    /* release the bucket */
    darray_release(bucket->brick);
    free(bucket);

    /* done! */
    return NULL;
}

void bucket_dtor_adapter(void* bucket)
{
    bucket_dtor((brickbucket_t*)bucket);
}

void bucket_add(brickbucket_t* bucket, brick_t* brick)
{
    darray_push(bucket->brick, brick);
}

int bucket_wash(brickbucket_t* bucket)
{
    int count = 0;

    /* remove dead bricks */
    for(int i = darray_length(bucket->brick) - 1; i >= 0; i--) {
        if(!brick_is_alive(bucket->brick[i])) {
            bucket->brick_dtor(bucket->brick[i]);
            darray_remove(bucket->brick, i);
            count++;
        }
    }

    /* return the number of removed bricks */
    return count;
}

void bucket_clear(brickbucket_t* bucket)
{
    for(int i = darray_length(bucket->brick) - 1; i >= 0 ; i--)
        bucket->brick_dtor(bucket->brick[i]);

    darray_clear(bucket->brick);
}

bool bucket_is_empty(const brickbucket_t* bucket)
{
    return 0 == darray_length(bucket->brick);
}



/* brick iterator state */

void* brickiteratorstate_copy_ctor(void* data)
{
    brickiteratorstate_t* state = mallocx(sizeof *state);
    memcpy(state, data, sizeof(*state));

    return state;
}

void brickiteratorstate_dtor(void* s)
{
    brickiteratorstate_t* state = (brickiteratorstate_t*)s;

    bucket_dtor(state->own_bucket);
    darray_release(state->bucket);

    free(state);
}

bool brickiteratorstate_has_next(void* s)
{
    brickiteratorstate_t* state = (brickiteratorstate_t*)s;
    int b = state->b;
    int i = state->i;

    return (b < darray_length(state->bucket)) &&
           (i < darray_length(state->bucket[b]->brick));
}

void* brickiteratorstate_next(void* s)
{
    brickiteratorstate_t* state = (brickiteratorstate_t*)s;
    int* b = &(state->b);
    int* i = &(state->i);

    /*

    Note that this approach does NOT filter out bricks that
    are stored in one of the buckets of the state, but that
    are outside of the ROI. A ROI test could be performed
    for each individual brick, but why do that? If the
    GRID_SIZE isn't too large, we can just pick bricks that
    are slightly outside the ROI - no problem! There aren't
    too many bricks.

    At the time of this writing, this is not an issue at all
    and we're interested in performance and in the simplicity
    of the code.

    Note: we do filter out individual bricks that are stored
    in the awake bucket, but we do it as a pre-processing step
    and with performance in mind. Bricks stored in the awake
    bucket may be far away from the ROI, and we don't need to
    return them.

    */

    if(*b < darray_length(state->bucket)) {
        int bucket_size = darray_length(state->bucket[*b]->brick);
        if(*i < bucket_size) {

            /* get the next element */
            brick_t* brick = state->bucket[*b]->brick[*i];

            /* advance the brick cursor */
            (*i)++;

            /* advance the bucket cursor */
            if(*i >= bucket_size) {
                *i = 0;
                (*b)++;
            }

            /* return the next element */
            return brick;

        }
        /*
        else {

            This must not happen.

            We expect no empty buckets in the iterator state.

        }
        */
    }

    /* there is no next element */
    return NULL;
}




/* height sampler */

heightsampler_t* sampler_ctor()
{
    heightsampler_t* sampler = mallocx(sizeof *sampler);

    darray_init(sampler->height_at);
    darray_init(sampler->smooth_height_at);

    darray_push(sampler->height_at, 0);
    darray_push(sampler->smooth_height_at, 0);

    return sampler;
}

heightsampler_t* sampler_dtor(heightsampler_t* sampler)
{
    darray_release(sampler->smooth_height_at);
    darray_release(sampler->height_at);
    free(sampler);

    return NULL;
}

void sampler_clear(heightsampler_t* sampler)
{
    darray_clear(sampler->height_at);
    darray_clear(sampler->smooth_height_at);

    darray_push(sampler->height_at, 0);
    darray_push(sampler->smooth_height_at, 0);
}

void sampler_add(heightsampler_t* sampler, const brick_t* brick)
{
    v2d_t spawn_point = brick_spawnpoint(brick);
    v2d_t size = brick_size(brick);

    int center_x = spawn_point.x + size.x * 0.5f;
    if(center_x < 0)
        center_x = 0;

    /* find the index corresponding to the brick */
    int index = center_x / SAMPLER_WIDTH;
    if(index > SAMPLER_MAX_INDEX)
        index = SAMPLER_MAX_INDEX; /* limit memory usage */

    /* ensure index < darray_length(sampler->height_at) */
    while(index >= darray_length(sampler->height_at))
        darray_push(sampler->height_at, 0); /* fill with zeros (meaning: no sampling data) */

    /* update height_at[] */
    int bottom = spawn_point.y + size.y;
    if(bottom > sampler->height_at[index])
        sampler->height_at[index] = bottom;

    /* fill smooth_height_at[] */
    /* assert(darray_length(sampler->smooth_height_at) >= 1); */
    for(int j = darray_length(sampler->smooth_height_at); j < darray_length(sampler->height_at); j++) {
        darray_push(sampler->smooth_height_at, 0);
        sampler->smooth_height_at[j] = sampler->smooth_height_at[j-1]; /* j >= 1 always */
    }

    /* update smooth_height_at[] */
    if(sampler->height_at[index] != 0)
        sampler->smooth_height_at[index] = sampler->height_at[index];
}

int sampler_query(heightsampler_t* sampler, int left, int right)
{
    /* invalid interval? */
    if(right < left)
        return 0;

    /* pick indices */
    int m = darray_length(sampler->smooth_height_at) - 1; /* m >= 0 always */
    int l = left / SAMPLER_WIDTH;
    int r = right / SAMPLER_WIDTH;

    if(l < 0)
        l = 0;
    if(l > m)
        l = m;

    if(r < 0)
        r = 0;
    if(r > m)
        r = m;
    
    /* now we have 0 <= l, r <= m */

    /* query the height at the given interval
       method: clamp to edge */
    int max_height = 0;
    for(int i = l; i <= r; i++) {
        if(sampler->smooth_height_at[i] > max_height)
            max_height = sampler->smooth_height_at[i];
    }

    /* done */
    return max_height;
}



/* world size */

void update_world_size(brickmanager_t* manager, const brick_t* brick)
{
    v2d_t spawn_point = brick_spawnpoint(brick);
    v2d_t size = brick_size(brick);

    int right = spawn_point.x + size.x;
    if(right > manager->world_width)
        manager->world_width = right;

    int bottom = spawn_point.y + size.y;
    if(bottom > manager->world_height)
        manager->world_height = bottom;
}



/* ROI & filtering */

bool is_brick_inside_roi(const brick_t* brick, const brickrect_t* roi)
{
    /* note that we use the position, which may change, instead of the spawn point! */
    v2d_t position = brick_position(brick);
    v2d_t size = brick_size(brick);

    int left = position.x;
    int top = position.y;
    int right = position.x + size.x - 1.0f;
    int bottom = position.y + size.y - 1.0f;

    return !(
        right < roi->left || left > roi->right ||
        bottom < roi->top || top > roi->bottom
    );
}

void filter_bricks_inside_roi(brickbucket_t* out_bucket, const brickbucket_t* in_bucket, const brickrect_t* roi)
{
    for(int i = 0; i < darray_length(in_bucket->brick); i++) {
        brick_t* brick = in_bucket->brick[i];

        if(is_brick_inside_roi(brick, roi))
            bucket_add(out_bucket, brick); /* add a reference to the output bucket */
    }
}

void filter_non_default_bricks(brickbucket_t* out_bucket, const brickbucket_t* in_bucket)
{
    for(int i = 0; i < darray_length(in_bucket->brick); i++) {
        brick_t* brick = in_bucket->brick[i];

        if(brick_behavior(brick) != BRB_DEFAULT)
            bucket_add(out_bucket, brick); /* add a reference to the output bucket */
    }
}


/* bucket brick destructor */

brick_t* brick_fake_destroy(brick_t* brick)
{
    /* do nothing. We'll just destroy a reference,
       not the brick itself */
    (void)brick;

    return NULL;
}



/* legacy brick list routines for backwards compatibility */

brick_list_t* add_to_list(brick_list_t* list, brick_t* brick)
{
    /* add quickly to the linked list */
    /* note that we're adding in reverse order */
    brick_list_t* node = mallocx(sizeof *node);
    node->data = brick;
    node->next = list;
    return node;
}

brick_list_t* release_list(brick_list_t* list)
{
    /* release linked list */
    while(list != NULL) {
        brick_list_t* next = list->next;
        free(list);
        list = next;
    }

    /* done */
    return NULL;
}