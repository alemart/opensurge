/*
 * Open Surge Engine
 * dnadoor.h - DNA doors
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

#ifndef _DNADOOR_H
#define _DNADOOR_H

#include "../item.h"

/* public methods */
item_t* surge_dnadoor_create(); /* regular DNA door: Surge */
item_t* neon_dnadoor_create(); /* DNA door: Neon */
item_t* charge_dnadoor_create(); /* DNA door: Charge */
item_t* surge_horizontal_dnadoor_create(); /* horizontal DNA door: Surge */
item_t* neon_horizontal_dnadoor_create(); /* horizontal DNA door: Neon */
item_t* charge_horizontal_dnadoor_create(); /* horizontal DNA door: Charge */

#endif
