/*
 * Open Surge Engine
 * elliptical_trajectory.h - This decorator makes the object follow an elliptical trajectory
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

#ifndef _OD_ELLIPTICALTRAJECTORY_H
#define _OD_ELLIPTICALTRAJECTORY_H

#include "base/objectdecorator.h"
#include "../../core/nanocalc/nanocalc.h"

/*
Please provide:

    amplitude_x     (in pixels)
    amplitude_y     (in pixels)
    angularspeed_x  (in revolutions per second)
    angularspeed_y  (in revolutions per second)
    initialphase_x  (in degrees)
    initialphase_y  (in degrees)
*/
objectmachine_t* objectdecorator_ellipticaltrajectory_new(objectmachine_t *decorated_machine, expression_t *amplitude_x, expression_t *amplitude_y, expression_t *angularspeed_x, expression_t *angularspeed_y, expression_t *initialphase_x, expression_t *initialphase_y);

#endif

