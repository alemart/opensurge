/*
 * Open Surge Engine
 * item.h - legacy items (replaced by SurgeScript)
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

#ifndef _ITEM_H
#define _ITEM_H

#include "../../util/v2d.h"

/* item list */
#define ITEMDATA_MAX        84 /* number of existing items */

#define IT_COLLECTIBLE      0  /* collectible */
#define IT_LIFEBOX          1  /* life box */
#define IT_COLLECTIBLEBOX   2  /* collectible box */
#define IT_STARBOX          3  /* invincibility stars */
#define IT_SPEEDBOX         4  /* turbo */
#define IT_GLASSESBOX       5  /* glasses */
#define IT_SHIELDBOX        6  /* shield */
#define IT_TRAPBOX          7  /* trap box */
#define IT_EMPTYBOX         8  /* empty box */
#define IT_CRUSHEDBOX       9  /* crushed box */
#define IT_ICON             10 /* box-related icon */
#define IT_FALGLASSES       11 /* UNUSED */
#define IT_EXPLOSION        12 /* explosion sprite */
#define IT_FLYINGTEXT       13 /* flying text */
#define IT_BOUNCINGCOLLECT  14 /* bouncing collectible */
#define IT_ANIMAL           15 /* little animal */
#define IT_LOOPRIGHT        16 /* loop right */
#define IT_LOOPMIDDLE       17 /* loop middle */
#define IT_LOOPLEFT         18 /* loop left */
#define IT_LOOPNONE         19 /* loop none */
#define IT_YELLOWSPRING     20 /* yellow spring */
#define IT_REDSPRING        21 /* red spring */
#define IT_RREDSPRING       22 /* right red spring */
#define IT_LREDSPRING       23 /* left red spring */
#define IT_BLUECOLLECTIBLE  24 /* blue collectible */
#define IT_SWITCH           25 /* switch */
#define IT_DOOR             26 /* door */
#define IT_TELEPORTER       27 /* teleporter */
#define IT_BIGRING          28 /* big ring */
#define IT_CHECKPOINT       29 /* checkpoint */
#define IT_GOAL             30 /* goal sign */
#define IT_ENDSIGN          31 /* end sign */
#define IT_ENDLEVEL         32 /* end level */
#define IT_LOOPFLOOR        33 /* loop floor */
#define IT_LOOPFLOORNONE    34 /* loop floor none */
#define IT_LOOPFLOORTOP     35 /* loop floor top */
#define IT_BUMPER           36 /* bumper */
#define IT_DANGER           37 /* danger */
#define IT_SPIKES           38 /* spikes */
#define IT_DNADOOR          39 /* DNA door: Surge */
#define IT_DANGPOWER        40 /* UNUSED */
#define IT_FIREBALL         41 /* UNUSED */
#define IT_FIRESHIELDBOX    42 /* fire shield */
#define IT_TRREDSPRING      43 /* top-right red spring */
#define IT_TLREDSPRING      44 /* top-left red spring */
#define IT_BRREDSPRING      45 /* bottom-right red spring */
#define IT_BLREDSPRING      46 /* bottom-left red spring */
#define IT_BREDSPRING       47 /* bottom red spring */
#define IT_RYELLOWSPRING    48 /* right yellow spring */
#define IT_LYELLOWSPRING    49 /* left yellow spring */
#define IT_TRYELLOWSPRING   50 /* top right yellow spring */
#define IT_TLYELLOWSPRING   51 /* top left yellow spring */
#define IT_BRYELLOWSPRING   52 /* bottom right yellow spring */
#define IT_BLYELLOWSPRING   53 /* bottom left yellow spring */
#define IT_BYELLOWSPRING    54 /* Bottom yellow spring */
#define IT_BLUESPRING       55 /* blue spring */
#define IT_RBLUESPRING      56 /* right blue spring */
#define IT_LBLUESPRING      57 /* left blue spring */
#define IT_TRBLUESPRING     58 /* top right blue spring */
#define IT_TLBLUESPRING     59 /* top left blue spring */
#define IT_BRBLUESPRING     60 /* bottom right blue spring */
#define IT_BLBLUESPRING     61 /* bottom left blue spring */
#define IT_BBLUESPRING      62 /* bottom blue spring */
#define IT_CEILSPIKES       63 /* spikes on the ceiling */
#define IT_LWSPIKES         64 /* spikes on left walls */
#define IT_RWSPIKES         65 /* spikes on right walls */
#define IT_PERSPIKES        66 /* periodic spikes on the floor */
#define IT_PERCEILSPIKES    67 /* periodic spikes on the ceiling */
#define IT_PERLWSPIKES      68 /* periodic spikes on left walls */
#define IT_PERRWSPIKES      69 /* periodic spikes on right walls */
#define IT_DNADOORNEON      70 /* DNA door: Neon */
#define IT_DNADOORCHARGE    71 /* DNA door: Charge */
#define IT_HDNADOOR         72 /* Horizontal DNA door: Surge */
#define IT_HDNADOORNEON     73 /* Horizontal DNA door: Neon */
#define IT_HDNADOORCHARGE   74 /* Horizontal DNA door: Charge */
#define IT_VDANGER          75 /* Vertical Danger */
#define IT_FIREDANGER       76 /* Fire Danger */
#define IT_VFIREDANGER      77 /* Vertical Fire Danger */
#define IT_THUNDERSHIELDBOX 78 /* Thunder shield box */
#define IT_WATERSHIELDBOX   79 /* Water shield box */
#define IT_ACIDSHIELDBOX    80 /* Acid shield box */
#define IT_WINDSHIELDBOX    81 /* Wind shield box */
#define IT_LOOPGREEN        82 /* activates the green layer */
#define IT_LOOPYELLOW       83 /* activates the yellow layer */

/* forward declarations */
struct actor_t;
struct player_t;
struct collisionmask_t;
struct brick_list_t;
struct item_list_t;
struct enemy_list_t;
typedef struct item_t item_t;
typedef struct item_list_t item_list_t;
typedef enum itemstate_t itemstate_t;

/* item state */
enum itemstate_t {
    IS_IDLE,          /* idle: default state */
    IS_DEAD           /* dead items are automatically removed from the item list */
};

/* <<abstract>> item class */
struct item_t {
    /* abstract methods */
    void (*init)(item_t*); /* initializes the item */
    void (*release)(item_t*); /* releases the item */
    void (*update)(item_t*, struct player_t**, int, struct brick_list_t*, item_list_t*, struct enemy_list_t*); /* updates the item (runs every frame) */
    void (*render)(item_t*, v2d_t); /* renders the item */

    /* properties of this item */
    struct actor_t* actor; /* actor */
    itemstate_t state; /* item state */
    int type; /* item type */
    int obstacle; /* is this item an obstacle? (i.e., is it not passable?) */
    int preserve; /* should we delete this item when it's outside the screen? */
    int bring_to_back; /* TODO: z-index?? */
    int always_active; /* always active? */
    struct collisionmask_t* mask; /* collision mask */
};

/* linked list of items */
struct item_list_t {
    item_t *data;
    item_list_t *next;
};

/* public functions: these are used by the external world */
item_t *item_create(int type); /* this is an item factory; type is an IT_* constant */
item_t* item_destroy(item_t *item);
void item_update(item_t *item, struct player_t** team, int team_size, struct brick_list_t *brick_list, struct item_list_t *item_list, struct enemy_list_t *enemy_list);
void item_render(item_t *item, v2d_t camera_position);

/* item-specific functions (legacy stuff) */
void bouncingcollectible_set_velocity(item_t *item, v2d_t velocity);
void flyingtext_set_text(item_t *item, const char *fmt, ...);

/* legacy objects that have been ported to SurgeScript */
const char* item2surgescript(int type);

#endif
