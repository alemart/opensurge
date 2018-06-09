/*
 * Open Surge Engine
 * spikes.h - spikes
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

#ifndef _SPIKES_H
#define _SPIKES_H

#include "../item.h"

/* public methods */
item_t* floorspikes_create(); /* floor spikes (the classic one) */
item_t* ceilingspikes_create(); /* ceiling spikes */
item_t* leftwallspikes_create(); /* left wall spikes */
item_t* rightwallspikes_create(); /* right wall spikes */

item_t* periodic_floorspikes_create(); /* floor spikes (moves up/down) */
item_t* periodic_ceilingspikes_create(); /* ceiling spikes */
item_t* periodic_leftwallspikes_create(); /* left wall spikes */
item_t* periodic_rightwallspikes_create(); /* right wall spikes */

#endif
