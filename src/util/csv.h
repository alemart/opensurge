/*
 * Open Surge Engine
 * csv.h - A simple utility for parsing CSV files
 * Copyright 2008-2025 Alexandre Martins <alemartf(at)gmail.com>
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

#ifndef _CSV_H
#define _CSV_H

/*
 * A CSV callback has the following signature:
 * void callback(int field_count, const char** fields, int line_number, void* user_data)
 * 
 * The callback is called for each line of the CSV file
 * Note: line_number starts counting from 0 - this is the CSV header
 */
typedef void (*csv_callback_t)(int,const char**,int,void*);

/* Parse a CSV file stored in memory */
void csv_parse(const char* csv_content, const char* delimiters, csv_callback_t callback, void* user_data);

#endif