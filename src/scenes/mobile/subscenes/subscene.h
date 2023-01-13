/*
 * Open Surge Engine
 * subscene.h - subscene definition
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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

#ifndef _MOBILESUBSCENE_H
#define _MOBILESUBSCENE_H

#include "../../../core/v2d.h"

typedef struct mobile_subscene_t mobile_subscene_t;
struct mobile_subscene_t {
    void (*init)(mobile_subscene_t*);
    void (*release)(mobile_subscene_t*);
    void (*update)(mobile_subscene_t*,v2d_t);
    void (*render)(mobile_subscene_t*,v2d_t);
};

#endif