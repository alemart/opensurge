/*
 * nanocalc addons
 * Mathematical built-in functions for nanocalc
 * Copyright (c) 2010, 2012  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this 
 * software and associated documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights to use, copy, modify, merge, 
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons 
 * to whom the Software is furnished to do so, subject to the following conditions:
 *   
 * The above copyright notice and this permission notice shall be included in all copies or 
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _NANOCALC_ADDONS_H
#define _NANOCALC_ADDONS_H

#ifdef __cplusplus
extern "C" {
#endif

/* calling this after nanocalc_init() will bind a lot of
   mathematical functions to the expression evaluator.
   You may take a look at nanocalc_addons.c to find out
   which functions are added, along with their interfaces */
void nanocalc_addons_init();

/* call this when you're done, but before nanocalc_release() */
void nanocalc_addons_release();

/* resets any arrays you may have */
void nanocalc_addons_resetarrays();

#ifdef __cplusplus
}
#endif

#endif
