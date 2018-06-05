/*
 * Open Surge Engine
 * stringutil.c - string utilities
 * Copyright (C) 2010, 2013  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "stringutil.h"
#include "util.h"

/*
 * str_to_hash()
 * Generates a hash key
 */
int str_to_hash(const char *str)
{
    int hash = 0;
    const char *p;

    for(p=str; *p; p++)
        hash = tolower(*p) + (hash << 6) + (hash << 16) - hash; /* we need tolower: hash key must be case insensitive */

    return hash;
}


/*
 * str_to_upper()
 * Upper-case convert
 */
const char *str_to_upper(const char *str)
{
    static char buf[1024];
    char *p;
    int i = 0;

    for(p=buf; *str && i<1023; str++,p++,i++)
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

    for(p=buf; *str && i<1023; str++,p++,i++)
        *p = tolower(*str);
    *p = '\0';

    return buf;
}


/*
 * str_icmp()
 * Case-insensitive compare function. Returns
 * 0 if s1==s2, -1 if s1<s2 or 1 if s1>s2.
 */
int str_icmp(const char *s1, const char *s2)
{
    const char *p, *q;
    char a, b;

    for(p=s1,q=s2; *p && *q; p++,q++) {
        a = tolower(*p);
        b = tolower(*q);
        if(a < b)
            return -1;
        else if(a > b)
            return 1;
    }

    if(!*p && *q)
        return -1;
    else if(*p && !*q)
        return 1;
    else
        return 0;
}



/*
 * str_cpy()
 * Safe version of strcpy(). Returns dest.
 * If we have something like char str[32], then
 * dest_size is 32, ie, sizeof(str)
 */
char* str_cpy(char *dest, const char *src, size_t dest_size)
{
    unsigned c;
    for(c = 0; (c < dest_size) && (dest[c] = src[c]); c++);
    dest[dest_size-1] = 0;
    return dest;
}



/*
 * str_trim()
 * Trim
 */
void str_trim(char *dest, const char *src)
{
    const char *p, *q, *t;

    for(p=src; *p && isspace((unsigned char)*p); p++);
    for(q=src+strlen(src)-1; q>p && isspace((unsigned char)*q); q--);
    for(q++,t=p; t!=q; t++) *(dest++) = *t;
    *dest = 0;
}



/*
 * str_dup()
 * Duplicates a string
 */
char *str_dup(const char *str)
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

    for(p=buf; *str && i<1023; str++,p++,i++) {
        if(*str == '"') {
            *p = '\\';
            if(++i<1023)
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
 * converts integer to string, returning a static string
 */
const char* str_from_int(int integer)
{
    static char buf[64];
    sprintf(buf, "%d", integer);
    return buf;
}
