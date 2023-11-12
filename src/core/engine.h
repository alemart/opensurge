/*
 * Open Surge Engine
 * engine.h - game engine facade
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

#ifndef _ENGINE_H
#define _ENGINE_H

#include <stdbool.h>
#include <allegro5/allegro.h>

void engine_init(int argc, char **argv);
void engine_mainloop();
void engine_release();
void engine_quit();

void engine_add_event_listener(ALLEGRO_EVENT_TYPE event_type, void* data, void (*callback)(const ALLEGRO_EVENT*,void*));
bool engine_remove_event_listener(ALLEGRO_EVENT_TYPE event_type, void* data, void (*callback)(const ALLEGRO_EVENT*,void*));
void engine_add_event_source(ALLEGRO_EVENT_SOURCE* event_source);
void engine_remove_event_source(ALLEGRO_EVENT_SOURCE* event_source);

uint32_t engine_game_id();
int engine_compatibility_version_code();

#endif

