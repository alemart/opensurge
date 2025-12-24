/*
 * Open Surge Engine
 * csv.c - A simple utility for parsing CSV files
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

#include <string.h>
#include "stringutil.h"
#include "csv.h"
#include "../core/logfile.h"

/* Maximum supported number of fields per line */
#define CSV_MAX_FIELDS 64

/*
 * csv_parse()
 * Parse a CSV file stored in memory
 */
void csv_parse(const char* csv_content, const char* delimiters, csv_callback_t callback, void* user_data)
{
    char* fields[CSV_MAX_FIELDS];
    int field_count = 0;
    int line_number = 0;

    /* copy the contents of the csv file */
    char* csv = str_dup(csv_content);

    /* for each line */
    char* line = strtok(csv, "\n");
    while(line != NULL) {

        /* for each line field */
        char* field = line;
        while(field != NULL) {
            /* store it in fields[] */
            char* end_of_field = strpbrk(field, delimiters);
            if(end_of_field != NULL) {
                char delim = *end_of_field;
                *end_of_field = 0;
                fields[field_count++] = str_dup(field);
                *end_of_field = delim;
                field = end_of_field + 1;
            }
            else {
                fields[field_count++] = str_dup(field);
                field = NULL;
            }

            /* too many fields? */
            if(field_count >= CSV_MAX_FIELDS) {
                logfile_message("Too many CSV fields (%d)", field_count);
                field = NULL;
            }
        }

        /* callback */
        callback(field_count, (const char**)fields, line_number++, user_data);

        /* clear up fields[] */
        while(field_count != 0)
            free(fields[--field_count]);

        /* read the next line */
        line = strtok(NULL, "\n");
    }

    /* release */
    free(csv);
}