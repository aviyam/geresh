// Copyright (C) 2003 Mooffie <mooffie@typo.co.il>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111, USA.

#include <config.h>

#include "utf8.h"
#include "univalues.h"
#include "dbg.h"

// utf8_to_unicode() - converts a UTF-8 string to unichars. When an
// incomplete sequence is encountered, *problem will point to its head (it's
// similar to what iconv does).
//
// This function converts UTF-8 to UCS-4 (not to UTF-32) -- that's why it
// recognizes 5- and 6-byte sequences.

int utf8_to_unicode(unichar *dest, const char *s, int len, const char **problem)
{
    int length = 0;
    const char *end = s + len;
    
    if (problem)
	*problem = NULL;

// constant expressions are evaluated at compile time, of course.
#define FRST(t)  ((*s & ((1 << (8-t-1)) - 1)) << (t-1)*6)
#define UC(t,n)  (*(s+n-1) & 0x3F) << ((t-n)*6)
    while (s < end) {
	if (!(*s & 0x80)) {
	    *dest++ = *s;
	    s++;
	} else if ((*s & 0xE0) == 0xC0) {
	    if ((end - s) >= 2) {
		*dest++ = FRST(2) | UC(2,2);
		s += 2;
	    } else {
		if (problem)
		    *problem = s;
		break;
	    }
	} else if ((*s & 0xF0) == 0xE0) {
	    if ((end - s) >= 3) {
		*dest++ = FRST(3) | UC(3,2) | UC(3,3);
		s += 3;
	    } else {
		if (problem)
		    *problem = s;
		break;
	    }
	} else if ((*s & 0xF8) == 0xF0) {
	    if ((end - s) >= 4) {
		*dest++ = FRST(4) | UC(4,2) | UC(4,3) | UC(4,4);
		s += 4;
	    } else {
		if (problem)
		    *problem = s;
		break;
	    }
	} else if ((*s & 0xFC) == 0xF8) {
	    if ((end - s) >= 5) {
		*dest++ = FRST(5) | UC(5,2) | UC(5,3) | UC(5,4) | UC(5,5);
		s += 5;
	    } else {
		if (problem)
		    *problem = s;
		break;
	    }
	} else if ((*s & 0xFE) == 0xFC) {
	    if ((end - s) >= 6) {
		*dest++ = FRST(6) | UC(6,2) | UC(6,3) | UC(6,4) | UC(6,5) | UC(6,6);
		s += 6;
	    } else {
		if (problem)
		    *problem = s;
		break;
	    }
	} else {
	    *dest++ = UNI_REPLACEMENT;
	    s++;
	}
	length++;
    }
    return length;
#undef FRST
#undef UC
}

// unicode_to_utf8() - converts unichars to UTF-8.

int unicode_to_utf8(char *dest, const unichar *us, int len)
{
#define UC(n)	((*us >> 6*n) & 0x3F)
#define CNT(n)	(((1 << n) - 1) << (8 - n))
    int nbytes = 0;
    while (len--) {
	if (*us < 0x80) {
	    *dest++ = *us;
	    nbytes += 1;
	} else if (*us < 0x800) {
	    *dest++ = UC(1) | CNT(2);
	    *dest++ = UC(0) | 0x80;
	    nbytes += 2;
	} else if (*us < 0x10000) {
	    *dest++ = UC(2) | CNT(3);
	    *dest++ = UC(1) | 0x80;
	    *dest++ = UC(0) | 0x80;
	    nbytes += 3;
	} else if (*us < 0x200000) {
	    *dest++ = UC(3) | CNT(4);
	    *dest++ = UC(2) | 0x80;
	    *dest++ = UC(1) | 0x80;
	    *dest++ = UC(0) | 0x80;
	    nbytes += 4;
	} else if (*us < 0x4000000) {
	    *dest++ = UC(4) | CNT(5);
	    *dest++ = UC(3) | 0x80;
	    *dest++ = UC(2) | 0x80;
	    *dest++ = UC(1) | 0x80;
	    *dest++ = UC(0) | 0x80;
	    nbytes += 5;
	} else {
	    *dest++ = UC(5) | CNT(6);
	    *dest++ = UC(4) | 0x80;
	    *dest++ = UC(3) | 0x80;
	    *dest++ = UC(2) | 0x80;
	    *dest++ = UC(1) | 0x80;
	    *dest++ = UC(0) | 0x80;
	    nbytes += 6;
	}
	us++;
    }
    return nbytes;
#undef UC
#undef CNT
}

