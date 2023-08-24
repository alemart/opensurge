/*
 * Open Surge Engine
 * stringutil.c - string utilities
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

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "stringutil.h"
#include "util.h"

/*
 * str_to_upper()
 * Convert to uppercase
 * If buffer is NULL, a statically allocated buffer is returned. Otherwise,
 * buffer is returned.
 */
char *str_to_upper(const char *str, char *buffer, size_t buffer_size)
{
    static char buf[1024];
    char *p;
    int i;

    if(buffer == NULL) {
        buffer = buf;
        buffer_size = sizeof buf;
    }

    for(p=buffer, i=0; *str && i<buffer_size-1; str++,p++,i++)
        *p = toupper(*str);
    *p = '\0';

    return buffer;
}



/*
 * str_to_lower()
 * Convert to lowercase
 * If buffer is NULL, a statically allocated buffer is returned. Otherwise,
 * buffer is returned.
 */
char *str_to_lower(const char *str, char *buffer, size_t buffer_size)
{
    static char buf[1024];
    char *p;
    int i = 0;

    if(buffer == NULL) {
        buffer = buf;
        buffer_size = sizeof buf;
    }

    for(p=buffer; *str && i<buffer_size-1; str++,p++,i++)
        *p = tolower(*str);
    *p = '\0';

    return buffer;
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
 * str_incmp()
 * Works like str_icmp(), except that this
 * function compares up to n characters
 */
int str_incmp(const char* s1, const char* s2, size_t n)
{
    const char *p, *q;
    int a, b;

    for(p=s1,q=s2; *p && *q && n--; p++,q++) {
        a = tolower(*p);
        b = tolower(*q);
        if(a != b)
            return a - b;
    }

    return 0;
}


/*
 * str_startswith()
 * Checks if str starts with the given prefix
 */
bool str_startswith(const char* str, const char* prefix)
{
    return (0 == strncmp(str, prefix, strlen(prefix)));
}


/*
 * str_endswith()
 * Checks if str ends with the given suffix
 */
bool str_endswith(const char* str, const char* suffix)
{
    size_t str_length = strlen(str);
    size_t suffix_length = strlen(suffix);

    return (str_length >= suffix_length) && (0 == strcmp(str + str_length - suffix_length, suffix));
}


/*
 * str_istartswith()
 * Checks if str starts with the given prefix, with a case-insensitive match
 */
bool str_istartswith(const char* str, const char* prefix)
{
    return (0 == str_incmp(str, prefix, strlen(prefix)));
}


/*
 * str_iendswith()
 * Checks if str ends with the given suffix, with a case-insensitive match
 */
bool str_iendswith(const char* str, const char* suffix)
{
    size_t str_length = strlen(str);
    size_t suffix_length = strlen(suffix);

    return (str_length >= suffix_length) && (0 == str_icmp(str + str_length - suffix_length, suffix));
}


/*
 * str_is_numeric()
 * Checks if str is numeric
 */
bool str_is_numeric(const char* str)
{
    const char* p = str;

    /* match +- */
    if(*p == '-' || *p == '+')
        str = ++p;

    /* match digits */
    while(isdigit(*p))
        p++;

    /* match . */
    if(*p == '.') {
        str = ++p;

        /* match digits */
        while(isdigit(*p))
            p++;
    }

    /* expect the end of the string */
    return (*p == '\0') && (p > str);
}


/*
 * str_is_boolean()
 * Checks if str is a boolean
 */
bool str_is_boolean(const char* str)
{
    return (0 == str_icmp(str, "true")) || (0 == str_icmp(str, "false"));
}


/*
 * str_cpy()
 * Safe version of strcpy(). Returns dest.
 * If we have something like char str[32], then
 * dest_size is 32, i.e., sizeof(str)
 */
char* str_cpy(char* dest, const char* src, size_t dest_size)
{
    strncpy(dest, src, dest_size);
    dest[dest_size - 1] = 0;
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
    char *p = mallocx((strlen(str) + 1) * sizeof(char));
    return strcpy(p, str);
}

/*
 * str_addslashes()
 * Replaces " by \\"
 * If buffer is NULL, a statically allocated buffer is returned. Otherwise,
 * buffer is returned.
 */
char* str_addslashes(const char *str, char *buffer, size_t buffer_size)
{
    static char buf[1024];
    int i = 0;
    char *p;

    if(buffer == NULL) {
        buffer = buf;
        buffer_size = sizeof buf;
    }

    for(p=buffer; *str && i<buffer_size-1; str++,p++,i++) {
        if(*str == '"') {
            *p = '\\';
            if(++i<buffer_size-1)
                *(++p) = *str;
        }
        else
            *p = *str;
    }
    *p = '\0';

    return buffer;
}

/*
 * str_normalize_slashes()
 * Replaces '\\' by '/' in-place
 */
char* str_normalize_slashes(char* str)
{
    if(str != NULL) {
        for(char* p = str; *p; p++) {
            if(*p == '\\')
                *p = '/';
        }
    }

    return str;
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


/*
 * x64_to_str()
 * Converts a uint64_t to a padded hex-string
 * If buf is NULL, an internal buffer is used and returned
 * If buf is not NULL, size should be at least 17
 */
char* x64_to_str(uint64_t value, char* buf, size_t size)
{
    static const char c[] = "0123456789abcdef";
    static char _buf[17] = "";
    char *p, *q, *r;

    /* use an internal buffer? */
    if(buf == NULL) {
        buf = _buf;
        size = sizeof(_buf);
    }

    /* validate the size of the buffer */
    else if(size < 17) {
        *buf = '\0';
        return buf;
    }

    /* write string */
    p = buf + (size = 17);
    *(--p) = '\0';
    while(--size) {
        *(--p) = c[value & 15];
        value >>= 4;
    }

    /* read leading zeros */
    for(q = p; *q == '0'; q++);

    /* remove leading zeros by moving the
       string to the beginning of the buffer */
    if(p != q) {
        for(r = p; *q != '\0';)
            *(r++) = *(q++);
        *r = '\0';
    }

    /* done */
    return p;
}


/*
 * str_to_x64()
 * Converts a hex-string to a uint64_t
 */
uint64_t str_to_x64(const char* buf)
{
    static const uint8_t t[128] = { /* accepts any 0-127 char */
        ['0'] = 0, ['1'] = 1, ['2'] = 2, ['3'] = 3, ['4'] = 4,
        ['5'] = 5, ['6'] = 6, ['7'] = 7, ['8'] = 8, ['9'] = 9,
        ['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15,
        ['A'] = 10, ['B'] = 11, ['C'] = 12, ['D'] = 13, ['E'] = 14, ['F'] = 15
    };
    uint64_t value = 0;

    while(*buf != '\0')
        value = (value << 4) | t[*(buf++) & 127];

    return value;
}