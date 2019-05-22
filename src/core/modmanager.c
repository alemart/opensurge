/*
 * Open Surge Engine
 * modmanager.c - MOD Manager
 * Copyright (C) 2018  Alexandre Martins <alemartf@gmail.com>
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
#include "modmanager.h"
#include "prefs.h"
#include "global.h"

/* private stuff */
static prefs_t* prefs = NULL;



/*
 * modmanager_init()
 * Initializes the MOD Manager
 */
void modmanager_init()
{
    prefs = prefs_create(NULL);
}

/*
 * modmanager_release()
 * Releases the MOD Manager
 */
void modmanager_release()
{
    prefs_destroy(prefs);
}

/*
 * modmanager_gameid()
 * Returns the game ID
 */
const char* modmanager_gameid()
{
    return GAME_UNIXNAME;
}

/*
 * modmanager_prefs()
 * Returns the prefs object related to the working MOD
 */
prefs_t* modmanager_prefs()
{
    return prefs;
}