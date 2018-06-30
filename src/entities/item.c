/*
 * Open Surge Engine
 * item.c - code for the items
 * Copyright (C) 2008-2010, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <stdlib.h>

#include "item.h"
#include "player.h"
#include "enemy.h"
#include "brick.h"
#include "actor.h"
#include "collisionmask.h"

#include "../core/global.h"
#include "../core/audio.h"
#include "../scenes/quest.h"
#include "../scenes/level.h"

#include "items/animal.h"
#include "items/animalprison.h"
#include "items/bigring.h"
#include "items/supercollectible.h"
#include "items/bouncingcollectible.h"
#include "items/bumper.h"
#include "items/checkpointorb.h"
#include "items/crushedbox.h"
#include "items/danger.h"
#include "items/dnadoor.h"
#include "items/door.h"
#include "items/endsign.h"
#include "items/explosion.h"
#include "items/flyingtext.h"
#include "items/goalsign.h"
#include "items/icon.h"
#include "items/itembox.h"
#include "items/loop.h"
#include "items/old_loop.h"
#include "items/collectible.h"
#include "items/spikes.h"
#include "items/spring.h"
#include "items/switch.h"
#include "items/teleporter.h"


/*
 * item_create()
 * Creates a new item
 */
item_t *item_create(int type)
{
    item_t *item = NULL;

    switch(type) {
        case IT_RING:
            item = collectible_create();
            break;

        case IT_BOUNCINGRING:
            item = bouncingcollectible_create();
            break;

        case IT_LIFEBOX:
            item = lifebox_create();
            break;

        case IT_RINGBOX:
            item = collectiblebox_create();
            break;

        case IT_STARBOX:
            item = starbox_create();
            break;

        case IT_SPEEDBOX:
            item = speedbox_create();
            break;

        case IT_GLASSESBOX:
            item = glassesbox_create();
            break;

        case IT_SHIELDBOX:
            item = shieldbox_create();
            break;

        case IT_FIRESHIELDBOX:
            item = fireshieldbox_create();
            break;

        case IT_THUNDERSHIELDBOX:
            item = thundershieldbox_create();
            break;

        case IT_WATERSHIELDBOX:
            item = watershieldbox_create();
            break;

        case IT_ACIDSHIELDBOX:
            item = acidshieldbox_create();
            break;

        case IT_WINDSHIELDBOX:
            item = windshieldbox_create();
            break;

        case IT_TRAPBOX:
            item = trapbox_create();
            break;

        case IT_EMPTYBOX:
            item = emptybox_create();
            break;

        case IT_CRUSHEDBOX:
            item = crushedbox_create();
            break;

        case IT_ICON:
            item = icon_create();
            break;

        case IT_EXPLOSION:
            item = explosion_create();
            break;

        case IT_FLYINGTEXT:
            item = flyingtext_create();
            break;

        case IT_ANIMAL:
            item = animal_create();
            break;

        case IT_LOOPRIGHT:
            item = loopright_create();
            break;

        case IT_LOOPMIDDLE:
            item = looptop_create();
            break;

        case IT_LOOPLEFT:
            item = loopleft_create();
            break;

        case IT_LOOPNONE:
            item = loopnone_create();
            break;

        case IT_LOOPFLOOR:
            item = loopfloor_create();
            break;

        case IT_LOOPFLOORNONE:
            item = loopfloornone_create();
            break;

        case IT_LOOPFLOORTOP:
            item = loopfloortop_create();
            break;

        case IT_YELLOWSPRING:
            item = yellowspring_create();
            break;

        case IT_BYELLOWSPRING:
            item = byellowspring_create();
            break;

        case IT_TRYELLOWSPRING:
            item = tryellowspring_create();
            break;

        case IT_RYELLOWSPRING:
            item = ryellowspring_create();
            break;

        case IT_BRYELLOWSPRING:
            item = bryellowspring_create();
            break;

        case IT_BLYELLOWSPRING:
            item = blyellowspring_create();
            break;

        case IT_LYELLOWSPRING:
            item = lyellowspring_create();
            break;

        case IT_TLYELLOWSPRING:
            item = tlyellowspring_create();
            break;

        case IT_REDSPRING:
            item = redspring_create();
            break;

        case IT_BREDSPRING:
            item = bredspring_create();
            break;

        case IT_TRREDSPRING:
            item = trredspring_create();
            break;

        case IT_RREDSPRING:
            item = rredspring_create();
            break;

        case IT_BRREDSPRING:
            item = brredspring_create();
            break;

        case IT_BLREDSPRING:
            item = blredspring_create();
            break;

        case IT_LREDSPRING:
            item = lredspring_create();
            break;

        case IT_TLREDSPRING:
            item = tlredspring_create();
            break;

        case IT_BLUESPRING:
            item = bluespring_create();
            break;

        case IT_BBLUESPRING:
            item = bbluespring_create();
            break;

        case IT_TRBLUESPRING:
            item = trbluespring_create();
            break;

        case IT_RBLUESPRING:
            item = rbluespring_create();
            break;

        case IT_BRBLUESPRING:
            item = brbluespring_create();
            break;

        case IT_BLBLUESPRING:
            item = blbluespring_create();
            break;

        case IT_LBLUESPRING:
            item = lbluespring_create();
            break;

        case IT_TLBLUESPRING:
            item = tlbluespring_create();
            break;

        case IT_BLUERING:
            item = supercollectible_create();
            break;

        case IT_SWITCH:
            item = switch_create();
            break;

        case IT_DOOR:
            item = door_create();
            break;

        case IT_TELEPORTER:
            item = teleporter_create();
            break;

        case IT_BIGRING:
            item = bigring_create();
            break;

        case IT_CHECKPOINT:
            item = checkpointorb_create();
            break;

        case IT_GOAL:
            item = goalsign_create();
            break;

        case IT_ENDSIGN:
            item = endsign_create();
            break;

        case IT_ENDLEVEL:
            item = animalprison_create();
            break;

        case IT_BUMPER:
            item = bumper_create();
            break;

        case IT_DANGER:
            item = horizontaldanger_create();
            break;

        case IT_VDANGER:
            item = verticaldanger_create();
            break;

        case IT_FIREDANGER:
            item = horizontalfiredanger_create();
            break;

        case IT_VFIREDANGER:
            item = verticalfiredanger_create();
            break;

        case IT_SPIKES:
            item = floorspikes_create();
            break;

        case IT_CEILSPIKES:
            item = ceilingspikes_create();
            break;

        case IT_LWSPIKES:
            item = leftwallspikes_create();
            break;

        case IT_RWSPIKES:
            item = rightwallspikes_create();
            break;

        case IT_PERSPIKES:
            item = periodic_floorspikes_create();
            break;

        case IT_PERCEILSPIKES:
            item = periodic_ceilingspikes_create();
            break;

        case IT_PERLWSPIKES:
            item = periodic_leftwallspikes_create();
            break;

        case IT_PERRWSPIKES:
            item = periodic_rightwallspikes_create();
            break;

        case IT_DNADOOR:
            item = surge_dnadoor_create();
            break;

        case IT_DNADOORNEON:
            item = neon_dnadoor_create();
            break;

        case IT_DNADOORCHARGE:
            item = charge_dnadoor_create();
            break;

        case IT_HDNADOOR:
            item = surge_horizontal_dnadoor_create();
            break;

        case IT_HDNADOORNEON:
            item = neon_horizontal_dnadoor_create();
            break;

        case IT_HDNADOORCHARGE:
            item = charge_horizontal_dnadoor_create();
            break;

        case IT_LOOPGREEN:
            item = loopgreen_create();
            break;

        case IT_LOOPYELLOW:
            item = loopyellow_create();
            break;
    }

    if(item != NULL) {
        item->type = type;
        item->state = IS_IDLE;
        item->init(item);
        item->mask = item->obstacle ? collisionmask_create(
            actor_image(item->actor),
            0,
            0,
            image_width(actor_image(item->actor)),
            image_height(actor_image(item->actor))
        ) : NULL;
    }

    return item;
}



/*
 * item_destroy()
 * Destroys an item
 */
item_t* item_destroy(item_t *item)
{
    if(item->mask != NULL)
        collisionmask_destroy(item->mask);
    item->release(item);
    free(item);
    return NULL;
}




/*
 * item_render()
 * Renders an item
 */
void item_render(item_t *item, v2d_t camera_position)
{
    item->render(item, camera_position);
}



/*
 * item_update()
 * Runs every cycle of the game to update an item
 */
void item_update(item_t *item, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, enemy_list_t *enemy_list)
{
    item->update(item, team, team_size, brick_list, item_list, enemy_list);
}

