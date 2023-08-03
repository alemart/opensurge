/*
 * Open Surge Engine
 * obstaclemap.c - physics system: obstacle map
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

#include "obstaclemap.h"
#include "obstacle.h"
#include "physicsactor.h"
#include "../core/video.h"
#include "../util/darray.h"
#include "../util/util.h"

/*

An obstacle map is a set of obstacles

Obstacles are placed in buckets distributed throughout the x-axis for efficient
access. Any particular obstacle may be placed in one or more buckets, depending
on its size. Buckets have fixed length. They are used to partition space. When
detecting collisions, we just inspect the obstacles of the relevant buckets.

*/
struct obstaclemap_t
{
    /* obstacles */
    DARRAY(const obstacle_t*, obstacle);

    /* possibly repeating obstacles sorted by increasing bucket index */
    DARRAY(const obstacle_t*, sorted_obstacle);

    /* cumulative sum of helper.bucket_count[] */
    DARRAY(int, bucket_start);

    /* number of buckets */
    int number_of_buckets;

    /* min limit of the obstacle map in the x-axis */
    int min_x;

    /* the obstacle map will be locked once we partition space */
    bool is_locked;

    /* helpers for the partitioning scheme with Counting Sort */
    struct {

        /* possibly repeating indices of obstacle[] in their incoming order */
        DARRAY(int, obstacle_index);

        /* bucket_index[i] is a bucket index of obstacle[obstacle_index[i]] */
        DARRAY(int, bucket_index);

        /* bucket_count[i] is the number of obstacles in the i-th bucket */
        DARRAY(int, bucket_count);

    } helper;
};

/*

The length of a bucket, in pixels

If you pick a number that is too large, the performance of the partitioning
scheme will be no better than that of brute force, because most or all
obstacles will be placed in the same bucket.

If you pick a number that is too small, obstacles will be repeated throughout
the buckets and we'll be inspecting them over and over again when detecting
collisions. The number of buckets will increase, and it should not exceed the
number of obstacles "too much" because of our building routine which relies on
Counting Sort, O(n+b). For example, the width of a typical sensor is too small.

I pick a reasonable size for a brick. That leads to a sensible partition and to
increased performance. It's a good idea to experiment with different values and
see the number of iterations performed by the collision detection routine, as
well as the number of buckets required by the partition.

*/
static const int BUCKET_LENGTH = 64; /* leads to a huge speedup compared to brute force (the actual factor also depends on the number of incoming obstacles, which depends on the settings of the Brick Manager) */
#define WANT_PERFORMANCE_REPORT 0 /* for testing only */

/*

Maximum number of buckets

The number of buckets at any given time is expected to be small and should not
exceed "too much" the number of obstacles. A maximum number of buckets is set
to limit memory usage and processing time.

The present method may not work as efficiently with disjoint and distant Region
of Interests (i.e., distinct clusters of objects), because many buckets will
end up empty and the algorithm will not give special treatment to such a sparse
setting. At the time of this writing, this is never the case: we stick to a
single rectangular ROI. Still, I'm documenting this for future reference.

Even with a sparse setting, the algorithm will perform better than simple brute
force. If we pick a large enough MAX_ROI_WIDTH that takes into account all
disjoint ROIs, but not too large so that MAX_BUCKETS explodes (relative to the
typical number of obstacles), we'll still get good performance, despite the
sparsity.

*/
static const int MAX_ROI_WIDTH = 16384; /* far beyond what's needed */
static const int MAX_BUCKETS = MAX_ROI_WIDTH / BUCKET_LENGTH;

/* private stuff */
static const int WORLD_LIMIT = LARGE_INT;
static const obstacle_t* pick_best_obstacle(const obstacle_t *a, const obstacle_t *b, int x1, int y1, int x2, int y2, movmode_t mm);
static inline bool ignore_obstacle(const obstacle_t *obstacle, obstaclelayer_t layer_filter);
static bool find_partition_limits(const obstaclemap_t* obstaclemap, int x1, int x2, int* begin, int* end);
static const obstacle_t* pick_tallest_ground(const obstacle_t* a, const obstacle_t* b, int x1, int y1, int x2, int y2, grounddir_t ground_direction, int* out_gnd);





/*
 * public functions
 */


/*
 * obstaclemap_create()
 * Create a new obstacle map
 */
obstaclemap_t* obstaclemap_create()
{
    obstaclemap_t *obstaclemap = mallocx(sizeof *obstaclemap);

    darray_init(obstaclemap->obstacle);
    darray_init(obstaclemap->sorted_obstacle);
    darray_init_ex(obstaclemap->bucket_start, MAX_BUCKETS + 1);

    obstaclemap->number_of_buckets = 0;
    obstaclemap->min_x = WORLD_LIMIT;
    obstaclemap->is_locked = false;

    darray_init(obstaclemap->helper.obstacle_index);
    darray_init(obstaclemap->helper.bucket_index);
    darray_init_ex(obstaclemap->helper.bucket_count, MAX_BUCKETS);

    return obstaclemap;
}

/*
 * obstaclemap_destroy()
 * Destroy an existing obstacle map
 */
obstaclemap_t* obstaclemap_destroy(obstaclemap_t *obstaclemap)
{
    darray_release(obstaclemap->helper.bucket_count);
    darray_release(obstaclemap->helper.bucket_index);
    darray_release(obstaclemap->helper.obstacle_index);

    darray_release(obstaclemap->bucket_start);
    darray_release(obstaclemap->sorted_obstacle);
    darray_release(obstaclemap->obstacle);

    free(obstaclemap);
    return NULL;
}

/*
 * obstaclemap_add()
 * Adds an obstacle to the obstacle map
 */
void obstaclemap_add(obstaclemap_t *obstaclemap, const obstacle_t *obstacle)
{
    /* can't add if locked */
    if(obstaclemap->is_locked) {
        fatal_error("Obstacle map is locked");
        return;
    }

    /* store the obstacle */
    darray_push(obstaclemap->obstacle, obstacle);

    /* update limit */
    int min_x = obstacle_get_position(obstacle).x;
    if(min_x < obstaclemap->min_x)
        obstaclemap->min_x = min_x;
}

/*
 * obstaclemap_clear()
 * Removes all obstacles from the obstacle map
 */
void obstaclemap_clear(obstaclemap_t* obstaclemap)
{
    darray_clear(obstaclemap->obstacle);
    darray_clear(obstaclemap->sorted_obstacle);
    darray_clear(obstaclemap->bucket_start);

    obstaclemap->number_of_buckets = 0;
    obstaclemap->min_x = WORLD_LIMIT;
    obstaclemap->is_locked = false; /* unlock */

    darray_clear(obstaclemap->helper.obstacle_index);
    darray_clear(obstaclemap->helper.bucket_index);
    darray_clear(obstaclemap->helper.bucket_count);
}

/*
 * obstaclemap_build()
 * Builds the internal data structure. Call after adding all obstacles
 */
void obstaclemap_build(obstaclemap_t* obstaclemap)
{
    /*

    We sort obstacles by increasing bucket index and in linear time using
    Counting Sort. This routine must be fast, as it runs on every frame.

    */
    int number_of_buckets = 0;
    int min_x = obstaclemap->min_x;

    /* quickly clear the arrays, just to be sure */
    darray_clear(obstaclemap->sorted_obstacle);
    darray_clear(obstaclemap->bucket_start);
    darray_clear(obstaclemap->helper.obstacle_index);
    darray_clear(obstaclemap->helper.bucket_index);
    darray_clear(obstaclemap->helper.bucket_count);

    /* for each obstacle j, normalize its x-position and find all relevant buckets */
    for(int j = 0; j < darray_length(obstaclemap->obstacle); j++) {
        const obstacle_t* obstacle = obstaclemap->obstacle[j];
        int x = obstacle_get_position(obstacle).x;
        int width = obstacle_get_width(obstacle);

        int normalized_x1 = x - min_x; /* never negative because min_x <= x */
        int normalized_x2 = (x + width - 1) - min_x; /* width >= 1 */

        int first_bucket = normalized_x1 / BUCKET_LENGTH;
        int last_bucket = normalized_x2 / BUCKET_LENGTH;

        /* checks and balances, just to be safe
           we should never need this for a typical Region of Interest */
        if(last_bucket > MAX_BUCKETS - 1)
            last_bucket = MAX_BUCKETS - 1;

        /* update the number of buckets
           we expect this to be a small integer
           the initial bucket of the obstacle map is zero */
        if(last_bucket + 1 > number_of_buckets)
            number_of_buckets = last_bucket + 1;

        /* associate obstacle j with buckets in { b | first_bucket <= b <= last_bucket } */
        for(int b = first_bucket; b <= last_bucket; b++) {
            darray_push(obstaclemap->helper.obstacle_index, j);
            darray_push(obstaclemap->helper.bucket_index, b);
        }
    }

    /* initialize bucket_count[] with zeros */
    for(int b = 0; b < number_of_buckets; b++)
        darray_push(obstaclemap->helper.bucket_count, 0);

    /* initialize sorted_obstacle[] */
    for(int i = 0; i < darray_length(obstaclemap->helper.obstacle_index); i++)
        darray_push(obstaclemap->sorted_obstacle, NULL);

    /* count the number of obstacles in each bucket */
    for(int i = 0; i < darray_length(obstaclemap->helper.bucket_index); i++) {
        int b = obstaclemap->helper.bucket_index[i];
        obstaclemap->helper.bucket_count[b]++;
    }

    /* compute the cumulative sum of bucket_count[] in-place
       we no longer need the original values */
    for(int b = 1; b < number_of_buckets; b++)
        obstaclemap->helper.bucket_count[b] += obstaclemap->helper.bucket_count[b-1];

    /* copy that cumulative sum to bucket_start[] for later use
       we make sure that the first entry is zero for convenience */
    darray_push(obstaclemap->bucket_start, 0);
    for(int b = 0; b < number_of_buckets; b++)
        darray_push(obstaclemap->bucket_start, obstaclemap->helper.bucket_count[b]);

    /* fill sorted_obstacle[] with Counting Sort */
    for(int i = darray_length(obstaclemap->helper.obstacle_index) - 1; i >= 0; i--) {
        int j = obstaclemap->helper.obstacle_index[i];
        int b = obstaclemap->helper.bucket_index[i];
        int k = --obstaclemap->helper.bucket_count[b];
        obstaclemap->sorted_obstacle[k] = obstaclemap->obstacle[j];
    }

    /* update the number of buckets in the structure
       and lock the obstacle map */
    obstaclemap->number_of_buckets = number_of_buckets;
    obstaclemap->is_locked = true;
}

/*
 * obstaclemap_get_best_obstacle_at()
 * Gets the "best" obstacle that hits a sensor, given a movmode_t and a layer
 * This routine assumes that the obstacle map is already built
 * It returns NULL if no hitting obstacle is found
 */
const obstacle_t* obstaclemap_get_best_obstacle_at(const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, movmode_t mm, obstaclelayer_t layer_filter)
{
    /*
    ************************************************************
    *** This routine is highly demanded and must be fast !!! ***
    ************************************************************
    */
    int begin, end;
    const obstacle_t *best = NULL;

    /* validate the input */
    if(x1 > x2 || y1 > y2)
        return NULL;

    /* find the limits of the partition */
    if(!find_partition_limits(obstaclemap, x1, x2, &begin, &end))
        return NULL; /* invalid partition */

    /* find the best obstacle */
    for(int j = begin; j < end; j++) { /* so simple and efficient!!! ;) */
        const obstacle_t *obstacle = obstaclemap->sorted_obstacle[j];

        if(!ignore_obstacle(obstacle, layer_filter) && obstacle_got_collision(obstacle, x1, y1, x2, y2))
            best = pick_best_obstacle(obstacle, best, x1, y1, x2, y2, mm);
    }

    /* done! */
    return best;
}

/*
 * obstaclemap_obstacle_exists()
 * Checks if an obstacle exists at (x,y)
 */
bool obstaclemap_obstacle_exists(const obstaclemap_t* obstaclemap, int x, int y, obstaclelayer_t layer_filter)
{
    int begin, end;

    /* find the limits of the partition */
    if(!find_partition_limits(obstaclemap, x, x, &begin, &end))
        return false; /* invalid partition */

    /* search for an obstacle */
    for(int j = begin; j < end; j++) {
        const obstacle_t *obstacle = obstaclemap->sorted_obstacle[j];

        if(!ignore_obstacle(obstacle, layer_filter) && obstacle_got_collision(obstacle, x, y, x, y))
            return true;
    }

    /* not found */
    return false;
}

/*
 * obstaclemap_solid_exists()
 * Checks if a solid obstacle exists at (x,y)
 */
bool obstaclemap_solid_exists(const obstaclemap_t* obstaclemap, int x, int y, obstaclelayer_t layer_filter)
{
    int begin, end;

    /* find the limits of the partition */
    if(!find_partition_limits(obstaclemap, x, x, &begin, &end))
        return false; /* invalid partition */

    /* search for a solid obstacle */
    for(int j = begin; j < end; j++) {
        const obstacle_t *obstacle = obstaclemap->sorted_obstacle[j];

        if(!ignore_obstacle(obstacle, layer_filter) && obstacle_got_collision(obstacle, x, y, x, y) && obstacle_is_solid(obstacle))
            return true;
    }

    /* not found */
    return false;
}

/*
 * obstaclemap_find_ground()
 * Find the tallest ground based on the specified parameters
 * We expect x1 <= x2 and y1 <= y2
 * Returns NULL if there is no ground
 */
const obstacle_t* obstaclemap_find_ground(const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, obstaclelayer_t layer_filter, grounddir_t ground_direction, int* out_ground_position)
{
    int begin, end;
    const obstacle_t *tallest_ground = NULL;

    /* validate the input */
    if(x1 > x2 || y1 > y2)
        return NULL;

    /* find the limits of the partition */
    if(!find_partition_limits(obstaclemap, x1, x2, &begin, &end))
        return NULL;

    /* find the tallest ground */
    for(int j = begin; j < end; j++) {
        const obstacle_t *obstacle = obstaclemap->sorted_obstacle[j];

        if(!ignore_obstacle(obstacle, layer_filter) && obstacle_got_collision(obstacle, x1, y1, x2, y2))
            tallest_ground = pick_tallest_ground(obstacle, tallest_ground, x1, y1, x2, y2, ground_direction, out_ground_position);
    }

    /* done! */
    return tallest_ground;
}



/* private methods */

/* given an interval I = [x1,x2], find maximal indices begin and end of sorted_obstacle[] such that
   sorted_obstacle[j] intersects with I for all j | begin <= j < end. Returns true on success. */
bool find_partition_limits(const obstaclemap_t* obstaclemap, int x1, int x2, int* begin, int* end)
{
    int min_x = obstaclemap->min_x;
    int number_of_buckets = obstaclemap->number_of_buckets;

    /* find the bucket range */
    int normalized_x1 = x1 - min_x;
    int normalized_x2 = x2 - min_x;

    int first_bucket = normalized_x1 / BUCKET_LENGTH;
    int last_bucket = normalized_x2 / BUCKET_LENGTH;

    /* clip */
    if(first_bucket < 0)
        first_bucket = 0;
    if(last_bucket >= number_of_buckets)
        last_bucket = number_of_buckets - 1;

    /* validate */
    if(first_bucket > last_bucket)
        return false; /* invalid [x1,x2] interval or number_of_buckets == 0 */

    /*

    Now that we have 0 <= first_bucket <= last_bucket < number_of_buckets,
    we find the relevant indices of sorted_obstacle[].

    Reminder: bucket_start[] has (number_of_buckets + 1) elements
              the first element is always zero!

    */
    *begin = obstaclemap->bucket_start[first_bucket];
    *end = obstaclemap->bucket_start[last_bucket + 1];

#if WANT_PERFORMANCE_REPORT
    /*

    how to tune performance:

    - change BUCKET_LENGTH
    - increase the speedup and decrease the bucket ratio
    - take into account the commentary about MAX_BUCKETS above

    */

    /* number of iterations with partitioning vs brute force */
    int partition = (*end) - (*begin);
    int brute_force = darray_length(obstaclemap->obstacle); /* test all obstacles */

    /* compute stats */
    float fraction = brute_force > 0 ? (float)partition / (float)brute_force : 0.0f;
    float bucket_ratio = brute_force > 0 ? (float)number_of_buckets / (float)brute_force : 0.0f;
    static const float alpha = 0.99f;
    static float smooth_fraction = 1.0f;
    smooth_fraction = smooth_fraction * alpha + (1.0f - alpha) * fraction;

    /* this speedup calculation does NOT measure how efficient it is to build
       the partition, which is dependent on the number of buckets, as well as
       on the number of obstacles */
    float speedup = smooth_fraction > 0.0f ? (1.0f / smooth_fraction) : 0.0f;

    /* report on screen */
    video_showmessage(
        "part=%d vs brute=%d | speedup=%.1fx | buckets=%d %.0f%%",
        partition,
        brute_force,
        speedup,
        number_of_buckets,
        100.0f * bucket_ratio
    );
#endif

    /* success! */
    return true;
}

/* considering that the sensor collides with both a and b, which one should we pick? */
/* we know that x1 <= x2 and y1 <= y2; these values already come rotated according to the movmode */
const obstacle_t* pick_best_obstacle(const obstacle_t *a, const obstacle_t *b, int x1, int y1, int x2, int y2, movmode_t mm)
{
    int x, y, ha, hb;
    bool sa, sb;

    /* NULL pointers should be handled */
    if(a == NULL)
        return b;
    if(b == NULL)
        return a;

    /* check the solidity of the obstacles */
    sa = obstacle_is_solid(a);
    sb = obstacle_is_solid(b);

    /* if both obstacles are solid, get the tallest obstacle */
    if(sa && sb) {
        switch(mm) {
            case MM_FLOOR:
                x = x2; /* x2 == x1 */
                y = y2; /* y2 == max(y1, y2) */
                ha = obstacle_ground_position(a, x, y, GD_DOWN);
                hb = obstacle_ground_position(b, x, y, GD_DOWN);
                return ha < hb ? a : b;

            case MM_RIGHTWALL:
                x = x2; /* x2 == max(x1, x2) */
                y = y1; /* y1 == y2 */
                ha = obstacle_ground_position(a, x, y, GD_RIGHT);
                hb = obstacle_ground_position(b, x, y, GD_RIGHT);
                return ha < hb ? a : b;

            case MM_CEILING:
                x = x2; /* x2 == x1 */
                y = y1; /* y1 == min(y1, y2) */
                ha = obstacle_ground_position(a, x, y, GD_UP);
                hb = obstacle_ground_position(b, x, y, GD_UP);
                return ha >= hb ? a : b;

            case MM_LEFTWALL:
                x = x1; /* x1 == min(x1, x2) */
                y = y1; /* y1 == y2 */
                ha = obstacle_ground_position(a, x, y, GD_LEFT);
                hb = obstacle_ground_position(b, x, y, GD_LEFT);
                return ha >= hb ? a : b;
        }
    }

    /* if both obstacles are one-way platforms, get the one with the shortest
       distance to the tail (x,y) of the sensor. The tail is likely in contact
       with an obstacle - in this case there won't be a discontinuity. */
    if(!sa && !sb) {
        switch(mm) {
            case MM_FLOOR:
                x = x2; /* x2 == x1 */
                y = y2; /* y2 == max(y1, y2) */
                ha = obstacle_ground_position(a, x, y, GD_DOWN);
                hb = obstacle_ground_position(b, x, y, GD_DOWN);
                return abs(ha - y) < abs(hb - y) ? a : b;

            case MM_RIGHTWALL:
                x = x2; /* x2 == max(x1, x2) */
                y = y1; /* y1 == y2 */
                ha = obstacle_ground_position(a, x, y, GD_RIGHT);
                hb = obstacle_ground_position(b, x, y, GD_RIGHT);
                return abs(ha - y) < abs(hb - y) ? a : b;

            case MM_CEILING:
                x = x1; /* x1 == x2 */
                y = y1; /* y1 = min(y1, y2) */
                ha = obstacle_ground_position(a, x, y, GD_UP);
                hb = obstacle_ground_position(b, x, y, GD_UP);
                return abs(ha - y) < abs(hb - y) ? a : b;

            case MM_LEFTWALL:
                x = x1; /* x1 = min(x1, x2) */
                y = y2; /* y2 == y1 */
                ha = obstacle_ground_position(a, x, y, GD_LEFT);
                hb = obstacle_ground_position(b, x, y, GD_LEFT);
                return abs(ha - y) < abs(hb - y) ? a : b;
        }
    }

    /* solid obstacles have preference over one-way platforms */
    return sa ? a : b;
}

/* pick the tallest ground between a and b. the sensor is assumed to collide with both. we assume x1 <= x2 and y1 <= y2 */
const obstacle_t* pick_tallest_ground(const obstacle_t* a, const obstacle_t* b, int x1, int y1, int x2, int y2, grounddir_t ground_direction, int* out_gnd)
{
    int x, y, ha, hb;

    /* we analyze from the HEAD (x,y) of the sensor because the sensor is
       assumed to be large and because the tail may surpass the ground */
    switch(ground_direction) {
        case GD_DOWN:   x = x1; y = y1; break; /* x1 == x2 and y1 == min(y1, y2) */
        case GD_UP:     x = x2; y = y2; break; /* x2 == x1 and y2 == max(y1, y2) */
        case GD_RIGHT:  x = x1; y = y1; break; /* x1 == min(x1, x2) and y1 == y2 */
        case GD_LEFT:   x = x2; y = y2; break; /* x2 == max(x1, x2) and y2 == y1 */
    }

    /* check for NULLs */
    if(a == NULL) {
        *out_gnd = obstacle_ground_position(b, x, y, ground_direction);
        return b;
    }

    if(b == NULL) {
        *out_gnd = obstacle_ground_position(a, x, y, ground_direction);
        return a;
    }

    /* which obstacle is the tallest? */
    ha = obstacle_ground_position(a, x, y, ground_direction);
    hb = obstacle_ground_position(b, x, y, ground_direction);
    switch(ground_direction) {
        case GD_DOWN:   *out_gnd = min(ha, hb); break;
        case GD_UP:     *out_gnd = max(ha, hb); break;
        case GD_RIGHT:  *out_gnd = min(ha, hb); break;
        case GD_LEFT:   *out_gnd = max(ha, hb); break;
    }
    return *out_gnd == ha ? a : b;
}

/* whether or not the given obstacle should be ignored, given a layer filter */
bool ignore_obstacle(const obstacle_t *obstacle, obstaclelayer_t layer_filter)
{
    obstaclelayer_t obstacle_layer = obstacle_get_layer(obstacle);
    return layer_filter != OL_DEFAULT && obstacle_layer != OL_DEFAULT && obstacle_layer != layer_filter;
}