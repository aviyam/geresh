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

#ifndef BDE_BIDI_H
#define BDE_BIDI_H

#include "types.h"

// The purpose of the BiDi class (and the public typdefs) is to make the
// interface of the actual BiDi engine (FriBiDi) hidden from the user. This
// BiDi class could be implemented using other engines, e.g. IBM's ICU.
//
// Ideally, The BiDi class would be an abstract class from which we derive
// class FriBiDi, class ICUBiDi etc. The configre script could then tell us
// which class to use based on what engine the system has.
//
// Note that we use very little of FriBiDi. We implement reordering ourselves
// (see the comments at EditBox::reorder()).

typedef FriBidiCharType	    ctype_t;
typedef FriBidiLevel	    level_t;
enum direction_t { dirLTR, dirRTL, dirN };
enum diralgo_t	 { algoUnicode, algoContextStrong, algoContextRTL, algoForceLTR, algoForceRTL };

#include "univalues.h"

class BiDi {

public:

    static bool mirror_char(unichar *ch) {
	return fribidi_get_mirror_char(*ch, ch);
    }

    static bool is_space(unichar ch) {
	return FRIBIDI_IS_SPACE(fribidi_get_type(ch)); 
    }

    static bool is_alnum(unichar ch) {
	ctype_t ctype = fribidi_get_type(ch);
	return FRIBIDI_IS_LETTER(ctype) || FRIBIDI_IS_NUMBER(ctype);
    }

    static bool is_nsm(unichar ch) {
	return fribidi_get_type(ch) == FRIBIDI_TYPE_NSM;
    }
    
    // is_wordch() returns what constitutes a word.
    static bool is_wordch(unichar ch) {
	return is_alnum(ch) || is_nsm(ch); 
    }
    
    static bool is_rtl(unichar ch) {
	return FRIBIDI_IS_RTL(fribidi_get_type(ch));
    }

    static bool is_explicit_mark(unichar ch)
    {
	return ch == UNI_LRM ||
	       ch == UNI_RLM ||
	       ch == UNI_LRE ||
	       ch == UNI_RLE ||
	       ch == UNI_PDF ||
	       ch == UNI_LRO ||
	       ch == UNI_RLO;
    }

    static bool is_transparent_formatting_code(unichar ch)
    {
	return is_explicit_mark(ch) ||
	       ch == UNI_ZWNJ ||
	       ch == UNI_ZWJ;
    }

    static bool is_hebrew_nsm(unichar ch)
    {
	return (ch >= UNI_HEB_ETNAHTA && ch <= UNI_HEB_UPPER_DOT
		    || ch == UNI_NS_UNDERSCORE)
		&& (ch != UNI_HEB_MAQAF &&
		    ch != UNI_HEB_PASEQ &&
		    ch != UNI_HEB_SOF_PASUQ);
    }

    static bool is_arabic_nsm(unichar ch)
    {
	return (ch >= UNI_ARA_FATHATAN && ch <= UNI_ARA_SUKUN
		    || ch == UNI_ARA_SUPERSCIPT_ALEF);
    }

    static bool is_cantillation_nsm(unichar ch)
    {
	return (ch >= UNI_HEB_ETNAHTA && ch <= UNI_HEB_MASORA_CIRCLE);
    }

    static void get_embedding_levels(unichar *str, idx_t len,
				     direction_t dir,
				     level_t *levels,
				     int line_breaks_count = 0,
				     idx_t *line_breaks = NULL,
				     bool disable_bidi = false);

    static direction_t determine_base_dir(unichar *str, idx_t len,
					  diralgo_t dir_algo);

    static void simple_log2vis(unistring &str, direction_t dir, unistring &dest);
};

#endif

