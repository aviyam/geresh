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

#include "iso88598.h"

#include "univalues.h"

#define ISO88598_HEB_ALEF   0xE0
#define ISO88598_HEB_TAV    0xFA

// The following 7 codes were taken from FriBiDi's fribidi_char_sets_iso8859_8.c.
// These are "proposed extensions to ISO-8859-8."

#define	ISO88598_LRM	    0xFD
#define ISO88598_RLM	    0xFE
#define ISO88598_LRE	    0xFB
#define ISO88598_RLE	    0xFC
#define ISO88598_PDF	    0xDD
#define ISO88598_LRO	    0xDB
#define ISO88598_RLO	    0xDC


// int unicode_to_iso88598(unichar ch)
//
// This is a fallback function used for saving files when no iconv
// implementation is present.
//
// It returns EOF when the unicode character can't be represented in
// ISO-8859-9.

int unicode_to_iso88598(unichar ch)
{
    if (ch <= 0xA0)
	return ch;
    if (ch >= UNI_HEB_ALEF && ch <= UNI_HEB_TAV)
	return ISO88598_HEB_ALEF + (ch - UNI_HEB_ALEF);
    switch (ch) {
	case 0x00D7:	      return 0xAA;   // MULTIPLICATION SIGN
	case 0x00F7:	      return 0xBA;   // DIVISION SIGN
	case 0x2017:	      return 0xDF;   // DOUBLE LOW LINE
    }
    if (ch >= 0xA2 && ch <= 0xBE)
	return ch;

    // The following are non-standard conversions.

    switch (ch) {
	case UNI_LRM: return ISO88598_LRM;
	case UNI_RLM: return ISO88598_RLM;
	case UNI_LRE: return ISO88598_LRE;
	case UNI_RLE: return ISO88598_RLE;
	case UNI_PDF: return ISO88598_PDF;
	case UNI_LRO: return ISO88598_LRO;
	case UNI_RLO: return ISO88598_RLO;
    }

    return EOF;
}

// unichar iso88598_to_unicode(unsigned char ch)
// 
// This is a fallback function used for loading files when no iconv
// implementation is present.
//
// It returns the Unicode Replacement Character when it encounters a
// character which is illegal in ISO-8859-9.

unichar iso88598_to_unicode(unsigned char ch)
{
    if (ch <= 0xA0)
	return ch;
    if (ch >= ISO88598_HEB_ALEF && ch <= ISO88598_HEB_TAV)
	return UNI_HEB_ALEF + (ch - ISO88598_HEB_ALEF);
    switch (ch) {
	case 0xAA: return 0x00D7;   // MULTIPLICATION SIGN
	case 0xBA: return 0x00F7;   // DIVISION SIGN
	case 0xDF: return 0x2017;   // DOUBLE LOW LINE
    }
    if (ch >= 0xA2 && ch <= 0xBE)
	return ch;

    // The following are non-standard conversions.

    switch (ch) {
	case ISO88598_LRM: return UNI_LRM;
	case ISO88598_RLM: return UNI_RLM;
	case ISO88598_LRE: return UNI_LRE;
	case ISO88598_RLE: return UNI_RLE;
	case ISO88598_PDF: return UNI_PDF;
	case ISO88598_LRO: return UNI_LRO;
	case ISO88598_RLO: return UNI_RLO;
    }

    return UNI_REPLACEMENT;
}

