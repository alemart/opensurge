/*
 * Open Surge Engine
 * entitytree.c - scripting system: Entity Tree for space partitioning
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

/*
 * SurgeEngine Entity Tree
 * =======================
 *            by alemart
 * 
 * Introduction
 * ------------
 * 
 * SurgeScript entities in world space are queried efficiently by means of
 * space partitioning. The 2D world is partitioned recursively into sectors,
 * which store other sectors and the entities. Since entities are queried and
 * updated on each frame of the game loop, we need an efficient method of
 * retrieval, so that we only update the entities that belong to a region of
 * interest, usually a rectangular window centered on the camera.
 * 
 * My method partitions the world using a tree, which represent sectors of
 * space in a way that fits naturally the SurgeScript object tree and that is
 * efficient. This tree which I describe is called the Entity Tree, also known
 * as the E.T. ;)
 * 
 * My method is not tied to SurgeScript entities, nor to this game engine. It's
 * a natural fit that can be implemented whenever you have in-game entities in
 * a scene graph. I explain the method in 2D, but it can be extended to 3D.
 * 
 * I consider this to be variant of a quadtree that is represented compactly,
 * that has a straightforward implementation, and that is designed for efficient
 * spatial search and storage.
 * 
 * 
 * 
 * Designing a tree for space partitioning
 * ---------------------------------------
 * 
 * Each sector of the world encompasses a rectangle (left, top, right, bottom)
 * of integer coordinates in world space. The world is itself bound by a
 * rectangle, (0, 0, world_width - 1, world_height - 1), where world_width
 * and world_height are positive integers given as input to the algorithm.
 * Coordinates are inclusive and measured in pixels. The size of the world may
 * change during gameplay, and the Entity Tree will adjust itself automatically.
 * The y-axis grows downwards in this implementation, but this doesn't have to
 * be the case, as this method is general.
 * 
 * Each sector has a unique integer i >= 0 associated with it. Sector 0 is
 * the root of the Entity Tree and encompasses the whole world. Furthermore,
 * a sector may be either a leaf or a non-leaf. Leaf sectors store entities.
 * Non-leaf sectors store other sectors. Such terminology is naturally mapped
 * to a tree structure.
 * 
 * Each non-leaf sector i is partitioned into 4 subsectors conveniently numbered
 * from 0 to 3: topleft (0), topright (1), bottomleft (2) and bottomright (3).
 * These subsectors are disjoint rectangles. They are called the children of i.
 * Similarly, sector i is called the parent of these subsectors. Leaf sectors
 * have no children.
 * 
 * 
 *                   +-----------------+-----------------+
 *                   |                 |                 |
 *                   |        0        |        1        |
 *                   |                 |                 |
 *                   +-----------------+-----------------+
 *                   |                 |                 |
 *                   |        2        |        3        |
 *                   |                 |                 |
 *                   +-----------------+-----------------+
 * 
 *                           A sector divided in 4
 * 
 * 
 * Given a sector i, the following formulas hold:
 * 
 * 
 * index of child j of i: (if i is a non-leaf sector)
 * 
 *     (1 + 4i + j), j = 0, 1, 2, 3
 * 
 * 
 * index of the parent of i: (if i is not the root, i.e., i != 0)
 * 
 *     floor((i-1) / 4)
 * 
 * 
 * The height H > 0 of the Entity Tree is constant throughout gameplay. It is
 * assumed to be a small integer. Each sector has a depth d, defined as its
 * distance to the root. Clearly, the root has depth zero, its children have
 * depth one, its grand-children have depth two, and so on. Each sector has
 * also a height h defined as h = H - d. All leaf sectors have height zero.
 * 
 * The level d of the Entity Tree is the set of sectors that have depth d.
 * Since each level has 4^d sectors, the number N of sectors of the tree
 * grows exponentially as a function of H:
 * 
 * 
 * N = 4^0 + 4^1 + ... + 4^H = ( 4^(H+1) - 1 ) / 3
 * 
 * 
 * The N sectors only exist in this theoretical model. In practice, we just
 * need a lazy allocation of the sectors. The 2D world is generally not
 * densely populated with entities.
 * 
 * Each sector has an address a = (p,d), which is a pair of integers. d is the
 * depth of the sector. p is called its path and describes how the sector can
 * be reached from the root. Given a sector index i, we can compute its address
 * as follows:
 * 
 * 
 * (p,d) = (0,0)
 * 
 * while i != 0:
 *     p = 4p + ((i-1) mod 4)
 *     i = floor((i-1) / 4)
 *     d = d + 1
 * 
 * return (p,d)
 * 
 * 
 * The address embodies a recursive structure. It can be computed quickly and
 * only once per sector.
 * 
 * Starting from the root, the two least significant bits of p determine the
 * first subsector to follow. The depth d determines the length of the path.
 * Therefore, given a sector address a = (p,d), we can locate it in the tree
 * as follows:
 * 
 * 
 * start from the root
 * 
 * while d != 0:
 *     take direction (p mod 4)
 *     p = floor(p / 4)
 *     d = d - 1
 * 
 * you're at your destination
 * 
 * 
 * We use the above strategy to find the coordinates in the cartesian plane of
 * any sector. Given a sector address a = (p,d) and integers world_width and
 * world_height, measured in pixels and assumed not to be smaller than 2^H,
 * we find the corresponding rectangle (left,top,right,bottom) = (l,t,r,b):
 * 
 * 
 * (l,t) = (0,0)
 * (r,b) = (world_width-1,world_height-1)
 * 
 * while d != 0:
 * 
 *     w = r - l + 1
 *     h = b - t + 1
 * 
 *     d = p mod 4
 *     p = floor(p / 4)
 *     d = d - 1
 * 
 *     if d == 0: // topleft
 * 
 *         r = l + ceil(w/2) - 1
 *         b = t + ceil(h/2) - 1
 * 
 *     else if d == 1: // topright
 * 
 *         l = r - floor(w/2) + 1
 *         b = t + ceil(h/2) - 1
 * 
 *     else if d == 2: // bottomleft
 * 
 *         r = l + ceil(w/2) - 1
 *         t = b - floor(h/2) + 1
 * 
 *     else: // bottomright
 * 
 *         l = r - floor(w/2) + 1
 *         t = b - floor(h/2) + 1
 * 
 * return (l,t,r,b)
 * 
 * 
 * The above algorithm partitions the (w,h)-sized sector into 4 subsectors.
 * The topleft and the bottomleft subsectors have width ceil(w/2). The others
 * have width floor(w/2). The left borders of the topleft and of the bottomleft
 * subsectors are equal to the left border of the parent sector. Similarly, the
 * right borders of the other two subsectors are equal to the right border of
 * the parent. An analogous argument holds for the heights: the topleft and the
 * topright subsectors have height ceil(h/2), and so on. Therefore, the parent
 * sector is divided into 4 disjoint rectangles.
 * 
 * Note that, for any integer k, the following expressions are valid:
 * 
 *     ceil(k/2) + floor(k/2) = k
 *     ceil(k/2) = floor((k+1)/2)
 *     floor(k/2) = k div 2 (integer division)
 * 
 *     proof sketch: k is either even or odd
 * 
 * Since each sector is divided into 4 disjoint rectangles, it follows that the
 * root sector, which encompasses the whole world, is partitioned into 4^H leaf
 * sectors: 2^H horizontally x 2^H vertically. If world_width is a power of 2,
 * say 2^k for some k >= H, then each leaf sector has width 2^(k-H). The height
 * computation is analogous. We would like to avoid leaf sectors of tiny size;
 * we will soon see why.
 * 
 * 
 * 
 * Maintaning the tree with bubbling
 * ---------------------------------
 * 
 * Entities are stored in the leaf sectors. Each entity has a position (x,y) of
 * integer coordinates in world space. These are assumed to be within the bounds
 * of the world (they can be clipped if necessary for the purpose of processing).
 * Each entity will be stored in the leaf sector whose rectangle contains its
 * position. Since leaf sectors are disjoint, it follows that there is only one
 * such sector.
 * 
 * Entities are frequently moving things, and they often move continuously. Not
 * only that: entities are commonly created and destroyed throughout the game.
 * We need to continuously and efficiently update the tree, so that the entities
 * are kept in their proper leaf sectors.
 * 
 * I introduce two operations, bubbleUp and bubbleDown, which will efficiently
 * keep the entities in their correct sectors. They will move the entities up
 * and down the tree. These operations are defined for all sectors of the tree
 * and they change depending on whether or not the sector is a leaf. These
 * operations take an entity as input.
 * 
 * The simplest operation is the bubbleDown for leaf sectors. Since they have
 * no child sectors, nothing can be moved down the tree and we can just store
 * the entity there:
 * 
 * 
 * bubbleDown(entity):
 * 
 *     store the entity in this sector
 * 
 * 
 * The bubbleUp operation for leaf sectors will remove an entity from a leaf
 * if it no longer belongs there. We say that an entity belongs to a sector
 * (leaf or non-leaf) if its position is contained in the rectangle of that
 * sector.
 * 
 * 
 * bubbleUp(entity):
 * 
 *     let parent be the parent sector
 * 
 *     if the entity does not belong to this sector:
 *         parent.bubbleUp(entity)
 * 
 * 
 * The bubbleUp operation for non-leaf sectors will check if the entity should
 * be moved up or down the tree:
 * 
 * 
 * bubbleUp(entity):
 * 
 *     let this be this sector and parent be the parent sector
 * 
 *     if the entity belongs to this sector:
 *         this.bubbleDown(entity)
 *     else:
 *         parent.bubbleUp(entity)
 * 
 * 
 * Since all entities are assumed to be within the boundaries of the world, the
 * root sector will never call bubbleUp.
 * 
 * Finally, operation bubbleDown for non-leaf sectors will find the appropriate
 * subsector for the input entity:
 * 
 * 
 * bubbleDown(entity):
 * 
 *     for each subsector of this sector:
 *         if the entity belongs to the subsector:
 *             lazily allocate the subsector
 *             subsector.bubbleDown(entity)
 *             return
 * 
 * 
 * Since the subsectors are disjoint, bubbleDown will be called only once. Note
 * that we don't allocate a subsector in advance to perform a belonging test.
 * 
 * These operations are performed quickly and moving an entity up and down the
 * tree is an efficient process.
 * 
 * Adding a new entity to the tree is quite straightforward. Let root be the
 * root sector. Then, we just call:
 * 
 * 
 * root.bubbleDown(entity)
 * 
 * 
 * The new entity will bubble down until it finds its proper leaf sector.
 * 
 * Removing an entity from the tree can be done directly: no need to move it.
 * 
 * Maintaining the entities in their proper sectors can be done via bubbleUp.
 * On each allocated leaf sector(*), run on every frame of the game loop:
 * 
 * 
 * for each entity stored in this sector:
 *     update the entity
 *     this.bubbleUp(entity)
 * 
 * 
 * (*) the number of allocated leaf sectors can be large, and thus we limit our
 * focus to the ones that intersect with a region of interest (ROI). We'll see
 * this in detail in the next sections.
 * 
 * A word of caution: if the leaf sectors are tiny, then the entities will
 * keep bubbling continuously. That's undesirable behavior. Suppose we have a
 * tiny world of 32x32 pixels with a tree height H = 5. In this case, each of
 * the 1024 leaf sectors have the size of a single pixel. If we have single
 * pixel entities moving around the tiny world, then they will keep bubbling on
 * each frame. Since sectors are allocated lazily, we would have plenty of
 * allocations taking place. The leaf sectors should be larger to avoid this
 * phenomenon. Picking H such that the size of the leaf sectors remains close
 * to the size of the typical region of interest (ROI) is a sensible heuristic,
 * though a bit limited: the size of the world changes and there is quite a
 * variance depending on the world that is being played. Still, if we know in
 * advance an average level size and the size of a typical ROI, then we can
 * pick H as follows:
 * 
 * H = ceil( log2(L / l) )
 * 
 * where L = max(world_width, world_height) and l = max(roi_width, roi_height).
 * 
 * Idea 1: we could modify the Entity Tree and relax the constraint that H must
 * be constant. Instead, we would set a minimum constant for H and let it grow.
 * If the number of entities stored in a leaf sector was beyond a threshold and
 * if that sector was "large enough", we could turn it into a non-leaf sector
 * and subdivide.
 * 
 * Idea 2: we could remove previously allocated leaf sectors that become empty
 * for a while (and non-leaf sectors as well if possible) in order to save
 * memory and processing time.
 * 
 * 
 * 
 * Finding entities in a region of interest
 * ----------------------------------------
 * 
 * A region of interest is defined as a rectangle R = (l,t,r,b). We would like
 * to quickly find all leaf sectors that intersect with R. Next, we'll filter
 * and return the entities.
 * 
 * Operation findIntersectingLeafSectors takes as parameters an output list L
 * and a region of interest R. We implement two variants: one for leaf sectors
 * and another for non-leaf sectors.
 * 
 * We start with the non-leaf variant:
 * 
 * 
 * findIntersectingLeafSectors(L, R):
 * 
 *     for each allocated subsector of this:
 *         if the subsector intersects with R:
 *             subsector.findIntersectingLeafSectors(L, R)
 * 
 * 
 * The routine above limits our search to the intersecting sectors. The leaf
 * variant picks the sectors and its implementation is trivial:
 * 
 * 
 * findIntersectingLeafSectors(L, R):
 * 
 *     add this leaf sector to L
 * 
 * 
 * In order to find the intersecting sectors in the entire tree, we just start
 * searching from the root, passing an empty output list L. Since H > 0, the
 * root is never a leaf, and so it doesn't matter if the ROI is not contained
 * in the boundaries of the world. The smaller the ROI, the faster the search.
 * 
 * After computing the leaf sectors that intersect with the ROI, selecting the
 * entities that are inside the ROI is trivial.
 * 
 * 
 * 
 * Updating the entities
 * ---------------------
 * 
 * In order to limit processing and gain performance, we focus on the entities
 * stored in the leaf sectors that intersect with the ROI. That's easy to do
 * with the routines we have developed so far:
 * 
 * 
 * update(R):
 * 
 *     let L be an empty list and root be the root sector
 * 
 *     root.findIntersectingLeafSectors(L, R)
 *     for each leaf sector of L:
 *         for each entity of the leaf sector:
 *             update the entity - if it belongs to R
 *             sector.bubbleUp(entity)
 * 
 *     return L
 * 
 * 
 * All we have to do now is call update(R), where R is our region of interest.
 * The intersecting leaf sectors are returned for convenience.
 * 
 */

#include <surgescript.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "scripting.h"
#include "../core/util.h"

typedef struct sector_t sector_t;
typedef struct sectoraddr_t sectoraddr_t;
typedef struct sectorrect_t sectorrect_t;
typedef struct sectorvtable_t sectorvtable_t;
typedef enum sectorquadrant_t sectorquadrant_t;
typedef surgescript_var_t* (*sectorfun_t)(surgescript_object_t*,const surgescript_var_t**,int);

/* the height of the quaternary tree - must be greater than zero
   the number of nodes in the tree grows exponentially (we allocate lazily) */
#define TREE_HEIGHT 5 /* log2(W / w); W = 32768 (max_level_width), w = 1024 (~roi_width) */

/* we use sensible constants, as tiny worlds would otherwise promote too much bubbling and memory allocations */
#define MIN_WORLD_WIDTH 8192 /* 2:1 ratio; 256x128 leaf area sector with H = 5; think about disposable entities */
#define MIN_WORLD_HEIGHT 4096

/* default world size */
#define DEFAULT_WORLD_WIDTH 32768 /* 2:1 ratio */
#define DEFAULT_WORLD_HEIGHT 16384 /* water at y ~ 10,000 */

enum sectorquadrant_t
{
    TOPLEFT = 0,
    TOPRIGHT = 1,
    BOTTOMLEFT = 2,
    BOTTOMRIGHT = 3
};

struct sectorrect_t
{
    /* coordinates are inclusive */
    int top;
    int left;
    int bottom;
    int right;
};

struct sectoraddr_t
{
    uint32_t path;
    int depth;
};

struct sectorvtable_t
{
    /* we use this vtable to bypass the SurgeScript
       call stack and gain extra speed */
    sectorfun_t bubble_up;
    sectorfun_t bubble_down;
    sectorfun_t update;
    sectorfun_t update_world_size;
};

struct sector_t
{
    int index;
    sectoraddr_t addr;

    bool is_leaf;
    const sectorvtable_t* vt;

    int cached_world_width;
    int cached_world_height;
    sectorrect_t cached_rect; /* depends on the size of the world */

    /* children info (no need of node allocation) */
    struct {
        int index;
        sectoraddr_t addr;
        sectorrect_t cached_rect;
    } child[4];
};

static sector_t* sector_ctor(int index, int world_width, int world_height);
static sector_t* sector_dtor(sector_t* sector);
static bool sector_update_rect(sector_t* sector, int world_width, int world_height);
static sectoraddr_t find_sector_address(int index);
static sectorrect_t find_sector_rect(sectoraddr_t addr, int world_width, int world_height);
static inline bool disjoint_rects(sectorrect_t a, sectorrect_t b);
static inline bool point_belongs_to_rect(sectorrect_t r, int x, int y);
static inline bool is_leaf_sector(int index);



static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_bubbleup(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_bubbledown(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_update(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_updateworldsize(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_leaf_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_leaf_bubbleup(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_leaf_bubbledown(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_leaf_update(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_leaf_updateworldsize(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t ENTITYCONTAINER_ADDR = 0; /* leaf nodes only */
static const surgescript_heapptr_t CHILD_ADDR[] = { /* non-leaf nodes only */
    [TOPLEFT] = 0,
    [TOPRIGHT] = 1,
    [BOTTOMLEFT] = 2,
    [BOTTOMRIGHT] = 3
};
#define unsafe_get_sector(tree_node) ((sector_t*)surgescript_object_userdata(tree_node))
static inline sector_t* safe_get_sector(surgescript_object_t* tree_node);
static surgescript_objecthandle_t spawn_child(surgescript_object_t* object, sectorquadrant_t quadrant);
static v2d_t get_clipped_position(surgescript_object_t* entity, float world_width, float world_height);

static const sectorvtable_t LEAF_VTABLE = {
    .bubble_up = fun_leaf_bubbleup,
    .bubble_down = fun_leaf_bubbledown,
    .update = fun_leaf_update,
    .update_world_size = fun_leaf_updateworldsize
};

static const sectorvtable_t NONLEAF_VTABLE = {
    .bubble_up = fun_bubbleup,
    .bubble_down = fun_bubbledown,
    .update = fun_update,
    .update_world_size = fun_updateworldsize
};





/*
 * scripting_register_entitytree()
 * Register the EntityTree object
 */
void scripting_register_entitytree(surgescript_vm_t* vm)
{
    /* EntityTree */
    surgescript_vm_bind(vm, "EntityTree", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "EntityTree", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "EntityTree", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "EntityTree", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "EntityTree", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "EntityTree", "bubbleUp", fun_bubbleup, 1);
    surgescript_vm_bind(vm, "EntityTree", "bubbleDown", fun_bubbledown, 1);
    surgescript_vm_bind(vm, "EntityTree", "update", fun_update, 5);
    surgescript_vm_bind(vm, "EntityTree", "updateWorldSize", fun_updateworldsize, 2);

    /* EntityTreeLeaf "inherits" from EntityTree */
    surgescript_vm_bind(vm, "EntityTreeLeaf", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "EntityTreeLeaf", "constructor", fun_leaf_constructor, 0);
    surgescript_vm_bind(vm, "EntityTreeLeaf", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "EntityTreeLeaf", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "EntityTreeLeaf", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "EntityTreeLeaf", "bubbleUp", fun_leaf_bubbleup, 1);
    surgescript_vm_bind(vm, "EntityTreeLeaf", "bubbleDown", fun_leaf_bubbledown, 1);
    surgescript_vm_bind(vm, "EntityTreeLeaf", "update", fun_leaf_update, 5);
    surgescript_vm_bind(vm, "EntityTreeLeaf", "updateWorldSize", fun_leaf_updateworldsize, 2);
}

/* constructor of a non-leaf node */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    const sector_t* sector = unsafe_get_sector(object);

    /* allocate root data */
    if(sector == NULL) {
        sector_t* root_sector = sector_ctor(0, DEFAULT_WORLD_WIDTH, DEFAULT_WORLD_HEIGHT);
        surgescript_object_set_userdata(object, root_sector);
    }

    /* children will be allocated lazily */
    for(int j = 0; j < 4; j++) {
        ssassert(CHILD_ADDR[j] == surgescript_heap_malloc(heap));
        surgescript_var_set_null(surgescript_heap_at(heap, CHILD_ADDR[j]));
    }

    /* done */
    return NULL;
}

/* constructor of a leaf node */
surgescript_var_t* fun_leaf_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const sector_t* sector = unsafe_get_sector(object);

    /* the sector is assumed to be allocated already
       (when spawning this node) */
    ssassert(sector != NULL); /* this object must not be spawned via SurgeScript! */

    /* spawn an EntityContainer */
    surgescript_objecthandle_t handle = surgescript_object_handle(object);
    surgescript_objecthandle_t container = surgescript_objectmanager_spawn(manager, handle, "EntityContainer", NULL);

    /* store the EntityContainer */
    ssassert(ENTITYCONTAINER_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, ENTITYCONTAINER_ADDR), container);

    /* done */
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    sector_t* sector = unsafe_get_sector(object);

    /* deallocate sector data */
    sector_dtor(sector);

    /* done */
    return NULL;
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* put it to sleep */
    surgescript_object_set_active(object, false);

    /* done */
    return NULL;
}

/* spawn function */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* destroy function */
surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* leaf-variant of update world size */
surgescript_var_t* fun_leaf_updateworldsize(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    double world_width = surgescript_var_get_number(param[0]);
    double world_height = surgescript_var_get_number(param[1]);
    sector_t* sector = unsafe_get_sector(object);

    /* update world size */
    if(!sector_update_rect(sector, world_width, world_height))
        return surgescript_var_set_bool(surgescript_var_create(), false);

    /* done */
    return surgescript_var_set_bool(surgescript_var_create(), true);
}

/* non-leaf-variant of update world size */
surgescript_var_t* fun_updateworldsize(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    double world_width = surgescript_var_get_number(param[0]);
    double world_height = surgescript_var_get_number(param[1]);
    sector_t* sector = unsafe_get_sector(object);

    /* update world size */
    if(!sector_update_rect(sector, world_width, world_height))
        return surgescript_var_set_bool(surgescript_var_create(), false); /* no change; return quickly */

    /* recurse on each allocated subsector */
    for(int j = 0; j < 4; j++) {
        surgescript_var_t* child_var = surgescript_heap_at(heap, CHILD_ADDR[j]);
        if(!surgescript_var_is_null(child_var)) {
            surgescript_objecthandle_t child_handle = surgescript_var_get_objecthandle(child_var);
            surgescript_object_t* child = surgescript_objectmanager_get(manager, child_handle);
            sector_t* child_sector = safe_get_sector(child);

            child_sector->vt->update_world_size(child, param, num_params);
        }
    }

    /* done */
    return surgescript_var_set_bool(surgescript_var_create(), true);
}

/* leaf-variant of bubble up */
surgescript_var_t* fun_leaf_bubbleup(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* get the entity */
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t entity_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);
    sector_t* sector = unsafe_get_sector(object);

    /* get the size of the world */
    int world_width = sector->cached_world_width;
    int world_height = sector->cached_world_height;

    /* does the entity belong to this sector? */
    v2d_t entity_position = get_clipped_position(entity, world_width, world_height);
    if(!point_belongs_to_rect(sector->cached_rect, entity_position.x, entity_position.y)) {

        /* call parent.bubbleUp(entity) */
        surgescript_objecthandle_t parent_handle = surgescript_object_parent(object);
        surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
        sector_t* parent_sector = safe_get_sector(parent);

        return parent_sector->vt->bubble_up(parent, param, num_params);

    }

    /* done */
    return NULL;
}

/* non-leaf-variant of bubble up */
surgescript_var_t* fun_bubbleup(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* get the entity */
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t entity_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);
    sector_t* sector = unsafe_get_sector(object);

    /* get the size of the world */
    int world_width = sector->cached_world_width;
    int world_height = sector->cached_world_height;

    /* does the entity belong to this sector? */
    v2d_t entity_position = get_clipped_position(entity, world_width, world_height);
    if(!point_belongs_to_rect(sector->cached_rect, entity_position.x, entity_position.y)) {

        /* call parent.bubbleUp(entity) */
        surgescript_objecthandle_t parent_handle = surgescript_object_parent(object);
        surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
        sector_t* parent_sector = safe_get_sector(parent);

        return parent_sector->vt->bubble_up(parent, param, num_params);

    }
    else {

        /* call this.bubbleDown(entity) */
        return sector->vt->bubble_down(object, param, num_params);

    }
}

/* leaf-variant of bubble down */
surgescript_var_t* fun_leaf_bubbledown(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* get the entity */
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t entity_handle = surgescript_var_get_objecthandle(param[0]);

    /* get the entity container of this leaf sector */
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* container_var = surgescript_heap_at(heap, ENTITYCONTAINER_ADDR);
    surgescript_objecthandle_t container_handle = surgescript_var_get_objecthandle(container_var);
    surgescript_object_t* container = surgescript_objectmanager_get(manager, container_handle);

    /* store the entity in the container of this sector */
    surgescript_var_t* arg = surgescript_var_create();
    const surgescript_var_t* args[] = { arg };

    surgescript_var_set_objecthandle(arg, entity_handle);
    surgescript_object_call_function(container, "reparent", args, 1, NULL);

    surgescript_var_destroy(arg);

    /* done */
    return NULL;
}

/* non-leaf-variant of bubble down */
surgescript_var_t* fun_bubbledown(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* get the entity */
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t entity_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);
    sector_t* sector = unsafe_get_sector(object);

    /* get the size of the world */
    int world_width = sector->cached_world_width;
    int world_height = sector->cached_world_height;

    /* get the position of the entity */
    v2d_t entity_position = get_clipped_position(entity, world_width, world_height);

    /* for each subsector */
    for(int j = 0; j < 4; j++) {

        /* does the entity belong to the j-th subsector? */
        if(point_belongs_to_rect(sector->child[j].cached_rect, entity_position.x, entity_position.y)) {

            /* lazily allocate the subsector */
            surgescript_var_t* child_var = surgescript_heap_at(heap, CHILD_ADDR[j]);
            if(surgescript_var_is_null(child_var)) {
                surgescript_objecthandle_t child_handle = spawn_child(object, j);
                surgescript_var_set_objecthandle(child_var, child_handle);
            }

            /* call subsector.bubbleDown(entity) */
            surgescript_objecthandle_t child_handle = surgescript_var_get_objecthandle(child_var);
            surgescript_object_t* child = surgescript_objectmanager_get(manager, child_handle);
            sector_t* child_sector = unsafe_get_sector(child);

            return child_sector->vt->bubble_down(child, param, num_params);

        }

    }

    /* this shouldn't happen */
    ssfatal(
        "Can't bubbleDown \"%s\" at (%f,%f) in [0-%lf)x[0-%lf)",
        surgescript_object_name(entity),
        entity_position.x, entity_position.y,
        world_width, world_height
    );
    return NULL;
}

/* non-leaf-variant of update: find intersecting leaf nodes */
surgescript_var_t* fun_update(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    /*surgescript_objecthandle_t array_handle = surgescript_var_get_objecthandle(param[0]);*/ /* ignored */
    double top = surgescript_var_get_number(param[1]);
    double left = surgescript_var_get_number(param[2]);
    double bottom = surgescript_var_get_number(param[3]);
    double right = surgescript_var_get_number(param[4]);
    sector_t* sector = unsafe_get_sector(object);

    sectorrect_t roi = {
        .top = top,
        .left = left,
        .bottom = bottom,
        .right = right
    };

    /* for each subsector */
    for(int j = 0; j < 4; j++) {

        /* does the subsector intersect with the ROI? */
        if(disjoint_rects(sector->child[j].cached_rect, roi))
            continue;

        /* is the subsector allocated? */
        const surgescript_var_t* child_var = surgescript_heap_at(heap, CHILD_ADDR[j]);
        if(surgescript_var_is_null(child_var))
            continue;

        /* recursion */
        surgescript_objecthandle_t child_handle = surgescript_var_get_objecthandle(child_var);
        surgescript_object_t* child = surgescript_objectmanager_get(manager, child_handle);
        sector_t* child_sector = unsafe_get_sector(child);

        child_sector->vt->update(child, param, num_params);

    }

    /* done */
    return NULL;
}

/* leaf-variant of update */
surgescript_var_t* fun_leaf_update(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t array_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* array = surgescript_objectmanager_get(manager, array_handle);

    /* get the entity container of this leaf sector */
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* container_var = surgescript_heap_at(heap, ENTITYCONTAINER_ADDR);
    surgescript_objecthandle_t container_handle = surgescript_var_get_objecthandle(container_var);
    surgescript_object_t* container = surgescript_objectmanager_get(manager, container_handle);

    /* update the entities */
    surgescript_object_traverse_tree(container, surgescript_object_update); /* the ROI test is done in the container */

    /* add the entity container of this leaf sector to the output array */
    surgescript_var_t* arg = surgescript_var_create();
    const surgescript_var_t* args[] = { arg };

    surgescript_var_set_objecthandle(arg, container_handle);
    surgescript_object_call_function(array, "push", args, 1, NULL);

    surgescript_var_destroy(arg);

    /* done */
    return NULL;
}



/*
 * private
 */

sectoraddr_t find_sector_address(int index)
{
    sectoraddr_t addr = { .path = 0, .depth = 0 };

    while(index > 0) {
        addr.path = 4 * addr.path + ((index - 1) % 4);
        addr.depth++;

        index = (index - 1) / 4;
    }

    return addr;
}

sectorrect_t find_sector_rect(sectoraddr_t addr, int world_width, int world_height)
{
    const int min_world_width = max(1 << TREE_HEIGHT, MIN_WORLD_WIDTH);
    const int min_world_height = max(1 << TREE_HEIGHT, MIN_WORLD_HEIGHT);

    if(world_width < min_world_width)
        world_width = min_world_width;
    if(world_height < min_world_height)
        world_height = min_world_height;

    sectorrect_t rect = {
        .left = 0,
        .top = 0,
        .right = world_width - 1,
        .bottom = world_height - 1
    };

    while(addr.depth != 0) {
        uint32_t direction = addr.path % 4;
        int w = rect.right - rect.left + 1;
        int h = rect.bottom - rect.top + 1;

        switch(direction) {
            case TOPLEFT:
                rect.right = rect.left + (w+1)/2 - 1;
                rect.bottom = rect.top + (h+1)/2 - 1;
                break;

            case TOPRIGHT:
                rect.left = rect.right - w/2 + 1;
                rect.bottom = rect.top + (h+1)/2 - 1;
                break;

            case BOTTOMLEFT:
                rect.right = rect.left + (w+1)/2 - 1;
                rect.top = rect.bottom - h/2 + 1;
                break;

            case BOTTOMRIGHT:
                rect.left = rect.right - w/2 + 1;
                rect.top = rect.bottom - h/2 + 1;
                break;
        }

        addr.path /= 4;
        addr.depth--;
    }

    return rect;
}

bool disjoint_rects(sectorrect_t a, sectorrect_t b)
{
    return b.right < a.left || b.left > a.right || b.bottom < a.top || b.top > a.bottom;
}

bool point_belongs_to_rect(sectorrect_t r, int x, int y)
{
    return r.left <= x && x <= r.right && r.top <= y && y <= r.bottom;
}

bool is_leaf_sector(int index)
{
    /*

    Each level d >= 0 of the Entity Tree has 4^d sectors indexed from first_d to
    last_d, inclusive. Clearly, level 0 has only one node, the root, and hence
    first_0 = last_d = 0. Since indices are always incremented by one, it follows
    that first_d = last_(d-1) + 1 for d > 0. Since level d has 4^d sectors, we have
    last_d - first_d + 1 = 4^d, or alternatively, last_d = last_(d-1) + 4^d for d > 0.
    We use the last equation to establish the following recurrence formula:

    l_d = { l_(d-1) + 4^d      if d > 0
          { 0                  if d = 0

    We solve analytically and find l_d = (4/3) * (4^d - 1) for d >= 0.

    If the Entity Tree has height H, then a sector is a leaf if its index is
    between first_H and last_H, inclusive. It's then easy to see that a sector
    is a leaf if:

    index > last_(H-1) = (4/3) * (4^(H-1) - 1)

    No invalid indices are provided as input (i.e., we assume index <= last_H).

    */
    const int last = 4 * (((1 << (2 * (TREE_HEIGHT - 1))) - 1) / 3);

    return index > last;
}

sector_t* sector_ctor(int index, int world_width, int world_height)
{
    sector_t* sector = mallocx(sizeof *sector);
    bool is_leaf = is_leaf_sector(index);

    sector->index = index;
    sector->addr = find_sector_address(index);

    sector->is_leaf = is_leaf;
    sector->vt = is_leaf ? &LEAF_VTABLE : &NONLEAF_VTABLE;

    sector->cached_world_width = 0;
    sector->cached_world_height = 0;
    sector->cached_rect = (sectorrect_t){ 0, 0, 0, 0 };

    if(!is_leaf) {
        for(int j = 0; j < 4; j++) {
            int child_index = 1 + 4 * index + j;
            sector->child[j].index = child_index;
            sector->child[j].addr = find_sector_address(child_index);
            sector->child[j].cached_rect = (sectorrect_t){ 0, 0, 0, 0 };
        }
    }
    else {
        /* fill with valid values */
        for(int j = 0; j < 4; j++) {
            sector->child[j].index = sector->index;
            sector->child[j].addr = sector->addr;
            sector->child[j].cached_rect = sector->cached_rect;
        }
    }

    sector_update_rect(sector, world_width, world_height);
    return sector;
}

sector_t* sector_dtor(sector_t* sector)
{
    free(sector);
    return NULL;
}

bool sector_update_rect(sector_t* sector, int world_width, int world_height)
{
    /* is the world too small? */
    const int min_world_width = max(1 << TREE_HEIGHT, MIN_WORLD_WIDTH);
    const int min_world_height = max(1 << TREE_HEIGHT, MIN_WORLD_HEIGHT);

    if(world_width < min_world_width)
        world_width = min_world_width;
    if(world_height < min_world_height)
        world_height = min_world_height;

    /* update only if needed */
    if(world_width != sector->cached_world_width || world_height != sector->cached_world_height) {
        sector->cached_world_width = world_width;
        sector->cached_world_height = world_height;
        sector->cached_rect = find_sector_rect(sector->addr, world_width, world_height);

        if(!is_leaf_sector(sector->index)) { /* is this test really needed? */
            for(int j = 0; j < 4; j++)
                sector->child[j].cached_rect = find_sector_rect(sector->child[j].addr, world_width, world_height);
        }
        else {
            for(int j = 0; j < 4; j++)
                sector->child[j].cached_rect = sector->cached_rect;
        }

        return true;
    }

    /* no need to update */
    return false;
}

sector_t* safe_get_sector(surgescript_object_t* tree_node)
{
    const char* object_name = surgescript_object_name(tree_node);

    if(0 == strcmp(object_name, "EntityTree") || 0 == strcmp(object_name, "EntityTreeLeaf"))
        return unsafe_get_sector(tree_node);

    /* this shouldn't happen */
    fatal_error("Can't get EntityTree sector of %s", object_name);
    return NULL;
}

surgescript_objecthandle_t spawn_child(surgescript_object_t* parent, sectorquadrant_t quadrant)
{
    const sector_t* parent_sector = unsafe_get_sector(parent);
    int world_width = parent_sector->cached_world_width;
    int world_height = parent_sector->cached_world_height;

    int child_index = 1 + 4 * parent_sector->index + quadrant; /* quadrant = 0, 1, 2, 3 */
    sector_t* child_sector = sector_ctor(child_index, world_width, world_height);
    const char* child_name = child_sector->is_leaf ? "EntityTreeLeaf" : "EntityTree";

    surgescript_objectmanager_t* manager = surgescript_object_manager(parent);
    surgescript_objecthandle_t parent_handle = surgescript_object_handle(parent);
    surgescript_objecthandle_t child_handle = surgescript_objectmanager_spawn(manager, parent_handle, child_name, child_sector);

    return child_handle;
}

v2d_t get_clipped_position(surgescript_object_t* entity, float world_width, float world_height)
{
    float x, y;

    if(world_width >= 1.0f && world_height >= 1.0f) {
        const surgescript_transform_t* transform = surgescript_object_transform(entity);

        surgescript_transform_getposition2d(transform, &x, &y); /* position in world space */

        x = clip(x, 0.0f, world_width - 1.0f);
        y = clip(y, 0.0f, world_height - 1.0f);
    }
    else {
        x = y = 0.0f;
    }

    return v2d_new(x, y);
}