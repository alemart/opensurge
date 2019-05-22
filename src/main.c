/*
 * Open Surge Engine
 * main.c - entry point
 * Copyright (C) 2008-2010  Alexandre Martins <alemartf@gmail.com>
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

#if !defined(A5BUILD)
#include <allegro.h>
#endif

#include "core/engine.h"

/*
 * main()
 * Entry point
 */
int main(int argc, char **argv)
{
    engine_init(argc, argv);
    engine_mainloop();
    engine_release();

    return 0;
}

#if !defined(A5BUILD)
END_OF_MAIN()
#endif

