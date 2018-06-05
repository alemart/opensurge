/*
 * Open Surge Engine
 * spring.h - spring
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _SPRING_H
#define _SPRING_H

#include "../item.h"

/* public methods */

/* yellow springs are weak */
item_t* yellowspring_create(); /* regular spring */
item_t* tryellowspring_create(); /* top-right */
item_t* ryellowspring_create();  /* right-oriented spring */
item_t* bryellowspring_create(); /* bottom-right */
item_t* byellowspring_create(); /* bottom-oriented spring */
item_t* blyellowspring_create(); /* bottom-left */
item_t* lyellowspring_create(); /* left-oriented spring */
item_t* tlyellowspring_create(); /* top-left */

/* red springs are strong */
item_t* redspring_create(); /* regular spring */
item_t* trredspring_create(); /* top-right */
item_t* rredspring_create();  /* right-oriented spring */
item_t* brredspring_create(); /* bottom-right */
item_t* bredspring_create(); /* bottom-oriented spring */
item_t* blredspring_create(); /* bottom-left */
item_t* lredspring_create(); /* left-oriented spring */
item_t* tlredspring_create(); /* top-left */

/* blue springs are the strongest */
item_t* bluespring_create(); /* regular spring */
item_t* trbluespring_create(); /* top-right */
item_t* rbluespring_create();  /* right-oriented spring */
item_t* brbluespring_create(); /* bottom-right */
item_t* bbluespring_create(); /* bottom-oriented spring */
item_t* blbluespring_create(); /* bottom-left */
item_t* lbluespring_create(); /* left-oriented spring */
item_t* tlbluespring_create(); /* top-left */

#endif
