/*
 * Open Surge Engine
 * stringutil.h - string utilities
 * Copyright 2008-2026 Alexandre Martins <alemartf(at)gmail.com>
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

#ifndef _STRINGUTIL_H
#define _STRINGUTIL_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

char* str_to_upper(const char* str, char* buffer, size_t buffer_size); /* convert to uppercase */
char* str_to_lower(const char* str, char* buffer, size_t buffer_size); /* convert to lowercase */

int str_icmp(const char* s1, const char* s2); /* case-insensitive compare function */
int str_incmp(const char* s1, const char* s2, size_t n); /* case-insensitive compare up to n characters */
bool str_startswith(const char* str, const char* prefix); /* checks if str starts with the given prefix */
bool str_endswith(const char* str, const char* suffix); /* checks if str ends with the given suffix */
bool str_istartswith(const char* str, const char* prefix); /* checks if str starts with the given prefix with a case-insensitive match */
bool str_iendswith(const char* str, const char* suffix); /* checks if str ends with the given suffix with a case-insensitive match */

int str_pathcmp(const char* s1, const char* s2); /* compare two file paths */
const char* str_pathextension(const char* filepath); /* the extension of a filepath, including the dot '.' */
bool str_pathhasextension(const char* filepath, const char* extension); /* checks if a filepath has an extension with the dot '.' */

bool str_is_numeric(const char* str); /* checks if str is numeric */
bool str_is_boolean(const char* str); /* checks if str is a boolean */

char* str_cpy(char* dest, const char* src, size_t dest_size); /* safe version of strcpy() */
char* str_trim(char* dest, const char* src, size_t dest_size); /* trim */
char* str_dup(const char* str); /* duplicates a string - strdup() isn't ANSI C */
char* str_rstr(char* haystack, const char* needle); /* reverse strstr() */

char* str_addslashes(const char* str, char* buffer, size_t buffer_size); /* replaces " by \\" */
char* str_normalize_slashes(char* str); /* replaces '\\' by '/' in-place */
const char* str_from_int(int integer, char* buffer, size_t buffer_size); /* converts integer to string; if no buffer is specified, returns a static char* */
const char* str_basename(const char* path); /* the filename of the path */
char* str_basename_without_extension(const char* path, char* buffer, size_t buffer_size); /* the filename of the path, without extension */
char* x64_to_str(uint64_t value, char* buf, size_t size); /* converts a 64-bit value to a padded hex-string */
uint64_t str_to_x64(const char* buf); /* converts a hex-string to a 64-bit value */

/* Macros */
#define atob(str)           (str_icmp((str), "true") == 0)

#endif