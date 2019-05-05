/*
 * Open Surge Engine
 * stringutil.c - string utilities
 * Copyright (C) 2010, 2013, 2019  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include <ctype.h>
#include <stdio.h>
#include "stringutil.h"
#include "util.h"

/*
 * str_to_upper()
 * Upper-case convert
 */
const char *str_to_upper(const char *str)
{
    static char buf[1024];
    char *p;
    int i;

    for(p=buf, i=0; *str && i<sizeof(buf)-1; str++,p++,i++)
        *p = toupper(*str);
    *p = '\0';

    return buf;
}



/*
 * str_to_lower()
 * Lower-case convert
 */
const char *str_to_lower(const char *str)
{
    static char buf[1024];
    char *p;
    int i = 0;

    for(p=buf; *str && i<sizeof(buf)-1; str++,p++,i++)
        *p = tolower(*str);
    *p = '\0';

    return buf;
}


/*
 * str_icmp()
 * Case-insensitive compare function. Returns
 * 0 if s1==s2, <0 if s1<s2 or >0 if s1>s2.
 */
int str_icmp(const char* s1, const char* s2)
{
    const char *p, *q;
    int a, b;

    for(p=s1,q=s2; *p && *q; p++,q++) {
        a = tolower(*p);
        b = tolower(*q);
        if(a != b)
            return a - b;
    }

    if(!*p && *q)
        return -1;
    else if(*p && !*q)
        return 1;
    else
        return 0;
}


/*
 * str_ihash()
 * Generates a case-insensitive hash key
 */
uint32_t str_ihash(const char *str)
{
    uint32_t hash = 0;

    while(*str) {
        /* we need to use tolower, as the hash key must be case insensitive */
        hash = tolower(*(str++)) + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}



/*
 * str_cpy()
 * Safe version of strcpy(). Returns dest.
 * If we have something like char str[32], then
 * dest_size is 32, i.e., sizeof(str)
 */
char* str_cpy(char* dest, const char* src, size_t dest_size)
{
    for(size_t c = 0; (c < dest_size) && (dest[c] = src[c]); c++);
    dest[dest_size-1] = 0;
    return dest;
}



/*
 * str_trim()
 * Trim string
 */
char* str_trim(char* dest, const char* src, size_t dest_size)
{
    const char *p, *q, *t;
    char *d;

    if(dest_size > 0) {
        for(p = src; *p && isspace((unsigned char)*p); p++);
        for(q = src + strlen(src) - 1; q != p && isspace((unsigned char)*q); q--);
        for(++q, t = p, d = dest; --dest_size && t != q; ) *(d++) = *(t++);
        *d = 0;
    }

    return dest;
}



/*
 * str_dup()
 * Duplicates a string
 */
char* str_dup(const char* str)
{
    char *p = mallocx((strlen(str)+1) * sizeof(char));
    strcpy(p, str);
    return p;
}

/*
 * str_addslashes()
 * replaces " by \\", returning a static char*
 */
const char* str_addslashes(const char *str)
{
    static char buf[1024];
    int i = 0;
    char *p;

    for(p=buf; *str && i<sizeof(buf)-1; str++,p++,i++) {
        if(*str == '"') {
            *p = '\\';
            if(++i<sizeof(buf)-1)
                *(++p) = *str;
        }
        else
            *p = *str;
    }
    *p = '\0';

    return buf;
}


/*
 * str_rstr()
 * Finds the last occurrence of needle in haystack. It's something
 * like strstr(), but reversed (i.e., "strrstr"). Returns NULL
 * if it doesn't find anything.
 */
char* str_rstr(char *haystack, const char *needle)
{
    if(*haystack) {
        char *p, *q;
        for(q = NULL; (p = strstr(haystack, needle)); q = p, haystack = p+1);
        return q;
    }
    else
        return NULL;
}


/*
 * str_from_int()
 * converts an integer to a string
 * If no buffer is supplied, returns a static string
 */
const char* str_from_int(int integer, char* buffer, size_t buffer_size)
{
    static char buf[32];
    if(buffer == NULL) {
        buffer = buf;
        buffer_size = sizeof(buf);
    }
    snprintf(buffer, buffer_size, "%d", integer);
    return buffer;
}


/*
 * str_basename()
 * returns the filename of the path 
 */
const char* str_basename(const char *path)
{
    const char *p = strpbrk(path, "\\/");
    while(p != NULL) {
        path = p+1;
        p = strpbrk(path, "\\/");
    }
    return path;
}