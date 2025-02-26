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

#include "bidi.h"
#include "dbg.h"

static ctype_t fribidi_dir(direction_t dir)
{
    switch (dir) {
    case dirLTR:  return FRIBIDI_TYPE_LTR;
    case dirRTL:  return FRIBIDI_TYPE_RTL;
    default:      return FRIBIDI_TYPE_ON;
    }
}

void BiDi::get_embedding_levels(unichar *str, idx_t len,
				direction_t dir,
				level_t *levels,
				int line_breaks_count,
				idx_t *line_breaks,
				bool disable_bidi)
{
    if (disable_bidi) {
	for (idx_t i = len-1; i >= 0; i--)
	    levels[i] = (dir == dirRTL) ? 1 :0;
    } else {
	ctype_t base_dir = fribidi_dir(dir);
	fribidi_log2vis_get_embedding_levels(str, len, &base_dir, levels);
    }

    // Do rule L1.4 of TR9: wspaces at end of lines.
    level_t para_embedding_level = (dir == dirRTL ? 1 : 0);
    int prev_line_break = 0;
    for (int i = 0; i < line_breaks_count; i++) {
	int pos = (int)line_breaks[i] - 1;
	while (pos >= prev_line_break && is_space(str[pos])) {
	    levels[pos] = para_embedding_level;
	    pos--;
	}
	prev_line_break = line_breaks[i];
    }
}

static void xsimple_log2vis(unichar *str, idx_t len, direction_t dir,
			    unichar *dest)
{
    ctype_t base_dir = fribidi_dir(dir);
    fribidi_log2vis(str, len, &base_dir, dest, NULL, NULL, NULL);
}

void BiDi::simple_log2vis(unistring &str, direction_t dir, unistring &dest)
{
    dest.resize(str.size() + 1); // fribidi_log2vis needs "+1" !!!!!!!!!!!!!
    xsimple_log2vis(str.begin(), str.size(), dir, dest.begin());
    dest.resize(str.size());
}

// determine_base_dir() - determines the base direction of a string.
// We provide the user with several algorithms. algoUnicode is described
// in P2,3 of TR9.
// 
// algoContext's implementation is divided into two: half is here and
// the other half is in EditBox::calc_contextual_dirs(). See documentation
// there.

direction_t BiDi::determine_base_dir(unichar *str, idx_t len,
				     diralgo_t dir_algo)
{
    ctype_t ctype;
    switch (dir_algo) {
	case algoContextRTL:
	{
	    // - if there's any RTL letter, set direction to LTR;
	    // - else: if there's any LTR letter, set direction to LTR;
	    // - else: set direction to neutral.
	    bool found_LTR = false;
	    for (idx_t i = 0; i < len; i++) {
		ctype = fribidi_get_type(str[i]);
		if (FRIBIDI_IS_LETTER(ctype)) {
		    if (FRIBIDI_IS_RTL(ctype))
			return dirRTL;
		    else
			found_LTR = true;
		}
	    }
	    if (found_LTR)
		return dirLTR;
	    else
		return dirN;
	}
	
	case algoUnicode:
	case algoContextStrong:
	{
	    // directionality is determined by the first strong letter.
	    for (idx_t i = 0; i < len; i++) {
		ctype = fribidi_get_type(str[i]);
		if (FRIBIDI_IS_LETTER(ctype)) {
		    if (FRIBIDI_IS_RTL(ctype))
			return dirRTL;
		    else
			return dirLTR;
		}
	    }
	    if (dir_algo == algoContextStrong)
		return dirN;
	    else
		return dirLTR;
	}

        case algoForceRTL:

	    return dirRTL;

	case algoForceLTR:
	default:

	    return dirLTR;
    };
}

