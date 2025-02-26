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

#ifndef BDE_UNIVALUES_H
#define BDE_UNIVALUES_H

// Line Separator and Paragraph Separator

#define UNICODE_LS	0x2028
#define UNICODE_PS	0x2029

// BIDI formatting codes

#define UNI_LRM		0x200E
#define UNI_RLM		0x200F
#define UNI_LRE		0x202A
#define UNI_RLE		0x202B
#define UNI_PDF		0x202C
#define UNI_LRO		0x202D
#define UNI_RLO		0x202E

// Hebrew codes, mainly points and punctuations

#define UNI_HEB_ALEF		    0x05D0
#define UNI_HEB_TAV		    0x05EA
#define UNI_HEB_GERESH		    0x05F3
#define UNI_HEB_GERSHAYIM	    0x05F4
#define UNI_HEB_SHEVA		    0x05B0
#define UNI_HEB_HATAF_SEGOL	    0x05B1
#define UNI_HEB_HATAF_PATAH	    0x05B2
#define UNI_HEB_HATAF_QAMATS	    0x05B3
#define UNI_HEB_HIRIQ		    0x05B4
#define UNI_HEB_TSERE		    0x05B5
#define UNI_HEB_SEGOL		    0x05B6
#define UNI_HEB_PATAH		    0x05B7
#define UNI_HEB_QAMATS		    0x05B8
#define UNI_HEB_HOLAM		    0x05B9
#define UNI_HEB_QUBUTS		    0x05BB
#define UNI_HEB_DAGESH_OR_MAPIQ	    0x05BC
#define UNI_HEB_METEG		    0x05BD
#define UNI_HEB_MAQAF		    0x05BE
#define UNI_HEB_RAFE		    0x05BF
#define UNI_HEB_PASEQ		    0x05C0
#define UNI_HEB_SHIN_DOT	    0x05C1
#define UNI_HEB_SIN_DOT		    0x05C2
#define UNI_HEB_SOF_PASUQ	    0x05C3
#define UNI_HEB_UPPER_DOT	    0x05C4

// Hebrew cantillation marks

#define UNI_HEB_ETNAHTA		    0x0591
#define UNI_HEB_MASORA_CIRCLE	    0x05AF
// in the above range, 0x05A2 is not allocated

// Arabic harakats
#define UNI_ARA_FATHATAN	    0x064B
#define UNI_ARA_SUKUN		    0x0652
#define UNI_ARA_SUPERSCIPT_ALEF	    0x0670

// Other punctuation

#define UNI_HYPHEN		    0x2010
#define UNI_NON_BREAKING_HYPHEN	    0x2011
#define UNI_EN_DASH		    0x2013
#define UNI_EM_DASH		    0x2014
#define UNI_LEFT_SINGLE_QUOTE	    0x2018
#define UNI_RIGHT_SINGLE_QUOTE	    0x2019
#define UNI_SINGLE_LOW9_QUOTE	    0x201A
#define UNI_SINGLE_HIGH_REV9_QUOTE  0x201B
#define UNI_LEFT_DOUBLE_QUOTE	    0x201C
#define UNI_RIGHT_DOUBLE_QUOTE	    0x201D
#define UNI_DOUBLE_LOW9_QUOTE	    0x201E
#define UNI_DOUBLE_HIGH_REV9_QUOTE  0x201F
#define UNI_MINUS_SIGN		    0x2212
#define UNI_BULLET		    0x2022

// Misc

#define UNI_REPLACEMENT		    0xFFFD
#define UNI_NS_UNDERSCORE	    0x0332
#define UNI_NO_BREAK_SPACE	    0x00A0

// Arabic

#define UNI_ZWNJ		    0x200C
#define UNI_ZWJ			    0x200D

#endif

