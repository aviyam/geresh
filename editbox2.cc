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

#include "editbox.h"
#include "transtbl.h"
#include "univalues.h"
#include "mk_wcwidth.h"
#include "my_wctob.h"
#include "shaping.h"
#include "themes.h"
#include "dbg.h"

// Default representations for some characters and concepts.
// If you want to change these, don't edit this file; edit "reprtab" instead.

#define FAILED_CONV_REPR '?'
#define CONTROL_REPR	'^'
#define NSM_REPR	'\''
#define WIDE_REPR	'A'
#define TRIM_REPR	'$'
#define WRAP_RTL_REPR	'/'
#define WRAP_LTR_REPR	'\\'

#define EOP_UNICODE_REPR    0xB6
#define EOP_DOS_REPR	    0xB5
#define EOP_UNIX_LTR_REPR   0xAB
#define EOP_UNIX_RTL_REPR   0xBB
#define EOP_MAC_REPR	    '@'
#define EOP_NONE_REPR	    0xAC

static bool do_mirror = true; // Do mirroring also when !bidi_enabled ?

// Setters and Togglers for various display options {{{

void EditBox::set_formatting_marks(bool value)
{
    show_paragraph_endings = value;
    show_explicits = value;
    show_tabs = value;
    rewrap_all(); // width changes because we show/hide explicit marks
    NOTIFY_CHANGE(formatting_marks);
}

// toggle_formatting_marks() - interactive command to toggle display of
// formatting marks.

INTERACTIVE void EditBox::toggle_formatting_marks()
{
    set_formatting_marks(!has_formatting_marks());
}

void EditBox::set_maqaf_display(maqaf_display_t disp)
{
    maqaf_display = disp;
    request_update(rgnAll);
    NOTIFY_CHANGE(maqaf);
}

// toggle_maqaf() - interactive command to toggle the display of maqaf.

INTERACTIVE void EditBox::toggle_maqaf()
{
    switch (maqaf_display) {
	case mqfAsis:		set_maqaf_display(mqfTransliterated); break;
	case mqfTransliterated:	set_maqaf_display(mqfHighlighted);    break;
	case mqfHighlighted:	set_maqaf_display(mqfAsis);	      break;
    }
}

bool EditBox::set_rtl_nsm_display(rtl_nsm_display_t disp)
{
    if (disp == rtlnsmAsis && (!terminal::is_utf8 || terminal::is_fixed)) {
	NOTIFY_ERROR(cant_display_nsm);
	return false;
    }
    rtl_nsm_display = disp;
    rewrap_all();
    NOTIFY_CHANGE(rtl_nsm);
    return true;
}

// toggle_rtl_nsm() - interactive command to toggle display of Hebrew
// points.

INTERACTIVE void EditBox::toggle_rtl_nsm()
{
    switch (rtl_nsm_display) {
    case rtlnsmOff:
	set_rtl_nsm_display(rtlnsmTransliterated);
	break;
    case rtlnsmTransliterated:
	if (set_rtl_nsm_display(rtlnsmAsis))
	    break;
	// fall-through
    case rtlnsmAsis:
	set_rtl_nsm_display(rtlnsmOff);
	break;
    }
}

void EditBox::enable_bidi(bool value)
{
    bidi_enabled = value;
    cache.invalidate();
    invalidate_optimal_vis_column();
    request_update(rgnAll);
}

INTERACTIVE void EditBox::toggle_bidi()
{
    enable_bidi(!is_bidi_enabled());
}

// }}}

// Updating {{{

// request_update() - request that some region be repainted next time the
// update() method is called.

void EditBox::request_update(region rgn)
{
    update_region |= rgn;
}

// request_update(from, to) - similar to request_update(rgnAll), but only
// paints the paragraphs in the range [from, to].

void EditBox::request_update(int from, int to)
{
    if (update_region & rgnRange) {
	// if there's already a pending rgnRange request,
	// give up and paint the whole window.
	update_region = rgnAll;
    } else {
	update_region |= rgnRange;
	update_from = from;
	update_to   = to;
    }
}

void EditBox::update()
{
    if (update_region == rgnNone)
	return;

    static int last_cursor_para = -1;
    if (is_primary_mark_set()) {
	// Determining which paragraphs to repaint when a selection is active
	// is a bit complicated, so we revert to a simple decision: if the
	// cursor hasn't moved from the paragraph we painted in the previous
	// update then the selection hasn't moved to include new paragraphs
	// and we can paint only this paragraph; else repaint the whole
	// window.
	if (last_cursor_para == cursor.para) {
	    // we repaint the whole paragraph even if we're asked to only
	    // reposition the cursor, because the selection has changed.
	    update_region |= rgnCurrent;
	} else {
	    update_region = rgnAll;
	}
	last_cursor_para = cursor.para;
    }

    if (wrap_type == wrpOff && update_region == rgnCursor) {
	// Well... this is a flaw in our update mechanism. In no-wrap mode we
	// repaint the whole paragraph even if we're asked to only reposition
	// the cursor. that's because the new cursor position might be in a
	// segment which is not shown on screen.
	update_region = rgnCurrent;
    }
	
    if (update_region & rgnAll) {
	wbkgd(wnd, get_attr(EDIT_ATTR));
	werase(wnd);
	// we invalidate the cache here instead of doing it after every change
	// that affects the display.
	cache.invalidate();
    }

    int window_line = -top_line.inner_line;
    int curr_para_line = 0; // the window line at which the current para starts.
			    // "=0" to silence the compiler.

    for (int i = top_line.para;
	     i < parags_count()
		&& window_line < window_height();
	     i++) {
        Paragraph *para = paragraphs[i];
	if (para == curr_para()) {
	    // we paint the current para outside of the loop because its
	    // painting also positions the cursor (which subsequent draws
	    // will invalidate).
	    curr_para_line = window_line;
	    if (update_region != rgnCursor) {
		// we don't paint it yet, but we erase its background
		for (int k = window_line;
			 k < window_line + para->breaks_count()
			    && k < window_height();
			 k++) {
		    wmove(wnd, k, 0);
		    wclrtoeol(wnd);
		}
	    }
	}
	else if ((update_region & rgnAll)
		 || ((update_region & rgnRange)
		      && i >= update_from && i <= update_to)) {
	    redraw_paragraph(*para, window_line, false, i);
	}
	window_line += para->breaks_count();
    }

    // paint the current paragraph
    bool only_cursor = (update_region == rgnCursor);
    redraw_paragraph(*curr_para(), curr_para_line, only_cursor, cursor.para);

    wnoutrefresh(wnd);
    update_region = rgnNone;
}

// }}}

// BiDi Reordering {{{

template <class VAL, class IDX>
inline void reverse(VAL *array, IDX len)
{
    if (len == 0)
	return; // IDX may be unsigned, so "-1" won't work.
    for (IDX start = 0, end = len - 1; start < end; start++, end--) {
	VAL tmp = array[start];
	array[start] = array[end];
	array[end] = tmp;
    }
}

// reorder() - Reorder Resolved Levels. Rules L2..L4. Unfortunately, FriBiDi
// has a flawed interface: this should be done after line-wrapping, and
// FriBiDi doesn't have a separate function for reordering, so we have to do
// it ourselves. (but it isn't so bad: that way we are not dependent on the
// BiDi engine, because we use very little of it.)

static void reorder(level_t *levels, idx_t len,
		    idx_t *position_V_to_L, idx_t *position_L_to_V,
		    unichar *str,
		    attribute_t *attributes = NULL,
		    bool mirror = false,
		    bool reorder_nsm = false)
{
    // We update a V_to_L vector from a L_to_V one. If the user
    // doesn't provide a L_to_V vector, we have to allocate it
    // ourselves.
    EditBox::IdxArray internal_position_V_to_L;

    if (position_L_to_V && !position_V_to_L) {
	internal_position_V_to_L.resize(len);
	position_V_to_L = internal_position_V_to_L.begin();
    }
    
    if (position_V_to_L) {
	for (idx_t i = 0; i < len; i++)
	    position_V_to_L[i] = i;
    }

    level_t highest_level = 0;
    level_t lowest_odd_level = 63;
    for (idx_t i = 0; i < len; i++) {
	level_t level = levels[i];
	if (level > highest_level)
	    highest_level = level;
	if ((level & 1) && level < lowest_odd_level)
	    lowest_odd_level = level;
    }

    // L2
    for (level_t level = highest_level; level >= lowest_odd_level; level--)
	for (idx_t i = 0; i < len; i++)
	    if (levels[i] >= level) {
		idx_t start = i;
		while (i < len && levels[i] >= level)
		    i++;

		if (position_V_to_L)
		    reverse(position_V_to_L + start, i - start);
		reverse(levels + start, i - start);
		if (str)
		    reverse(str + start, i - start);
		if (attributes)
		    reverse(attributes + start, i - start);
	    }
    
    // L3, L4: Mirroring and NSM reordering.
    if (str && (mirror || reorder_nsm)) {
	for (idx_t i = 0; i < len; i++) {
	    if (levels[i] & 1) {
		BiDi::mirror_char(&str[i]);
		// we don't unconditionally reorder NSMs bacause when the
		// user prefers ASCII transliteration they should be left
		// to the left of the character.
		if (reorder_nsm
			&& BiDi::is_nsm(str[i]))
		{
		    idx_t start = i++;
		    while (i < len && BiDi::is_nsm(str[i]))
			i++;
		    if (i < len)    // :FIXME: only if on the same level.
			i++;	    // include the base character.

		    // should I reorder position_V_to_L and attributes too?
		    //if (position_V_to_L)
		    //	reverse(position_V_to_L + start, i - start);
		    reverse(str + start, i - start);
		    //if (attributes)
		    //	reverse(attributes + start, i - start);
		    if (i < len)
			i--;
		}
	    }
	}
    }

    if (position_L_to_V) {
	for (idx_t i = 0; i < len; i++)
	    position_L_to_V[position_V_to_L[i]] = i;
    }
}

// }}}

// Wrap / character traits {{{

// most of the characters will probably be "simple" ones, meaning
// they occupy one terminal column. IS_SIMPLE_ONE_COLUMN_CHAR helps
// us avoid a call to wcwidth().

#define IS_SIMPLE_ONE_COLUMN_CHAR(ch) \
    ((ch >= 32 && ch <= 126) \
	|| (ch >= UNI_HEB_ALEF && ch <= UNI_HEB_TAV))

// get_char_width() - returns the width, in terminal columns, of a character.
// it takes into account the user's preferences (e.g. whether to show explicit
// marks), LAM-ALEF ligature, and the terminal capabilities (e.g. whether it's
// capable of displaying wide and combining characters). it also calculates
// the width of a TAB character based on its position in the line (pos).

int EditBox::get_char_width(unichar ch, int pos, wdstate *stt, bool visual)
{
#define LAM_N			0x0644
#define IS_LAM_N(c) ((c) == LAM_N)

#define ALEF_MADDA_N		0x0622
#define ALEF_HAMZA_ABOVE_N	0x0623
#define ALEF_HAMZA_BELOW_N	0x0625
#define ALEF_N			0x0627
#define IS_ALEF_N(c) ((c) == ALEF_MADDA_N \
			|| (c) == ALEF_HAMZA_ABOVE_N \
			|| (c) == ALEF_HAMZA_BELOW_N \
			|| (c) == ALEF_N)

    // optimization
    if (IS_SIMPLE_ONE_COLUMN_CHAR(ch) && !(stt && stt->may_start_lam_alef))
	return 1;
    
    int width = mk_wcwidth(ch);

    // handle ALEF-LAM ligature
    if (terminal::do_arabic_shaping && stt) {
	if (visual ? IS_ALEF_N(ch) : IS_LAM_N(ch)) {
	    stt->may_start_lam_alef = true;
	} else {
	    if (stt->may_start_lam_alef
		    && (visual ? IS_LAM_N(ch) : IS_ALEF_N(ch))) {
		// yes, this is the second character of the ligature.
		// return 0 (the ligature width is 1, and the first
		// character already returned 1).
		stt->may_start_lam_alef = false;
		return 0;
	    } else {
		if (width != 0 || !is_shaping_transparent(ch))
		    stt->may_start_lam_alef = false;
	    }
	}
    }

    if (width != 1) {
	if (ch == '\t') {
	    return (tab_width - (pos % tab_width));
	} else if (width == -1 || ch == '\0') {
	    // We print control characters and nulls on one column, so
	    // we set their width to 1.
	    width = 1;
	} else if (width == 0) {
	    // Calculate required width for BIDI codes and Hebrew points
	    // based on user's preferences.
	    if (BiDi::is_explicit_mark(ch)) {
		if (show_explicits)
		    width = 1;
	    } else if (BiDi::is_hebrew_nsm(ch) || BiDi::is_arabic_nsm(ch)) {
		if (rtl_nsm_display == rtlnsmTransliterated)
		    width = 1;
	    } else {
		// Now handle general NSMs:
		// if it's not a UTF-8 terminal, assume it can't combine
		// characters. Same for fixed terminals.
		if (!terminal::is_utf8 || terminal::is_fixed)
		    width = 1;
	    }
	} else {
	    // width == 2, Asian ideographs
	    if (!terminal::is_utf8 || terminal::is_fixed)
		width = 1;
	}
    }

    return width;
}

int EditBox::get_str_width(const unichar *str, idx_t len, bool visual)
{
    wdstate stt;
    int width = 0;
    while (len--)
	width += get_char_width(*str++, width, &stt, true);
    return width;
}

// get_rev_str_width() - sums the widths of the characters starting at the
// end of the string. it makes a difference only for the TAB character.

int EditBox::get_rev_str_width(const unichar *str, idx_t len)
{
    wdstate stt;
    int width = 0;
    str += len;
    while (len--)
	width += get_char_width(*(--str), width, &stt);
    return width;
}

// calc_vis_column() - returns the visual column the cursor is at.
// It does a logical-to-visual conversion, then it sums, in visual order, 
// the widths of the characters till the cursor.

int EditBox::calc_vis_column()
{
    Paragraph &p = *curr_para();

    // Find the start and the end of the screen line.
    int inner_line = calc_inner_line();
    idx_t start = (inner_line > 0)
		    ? p.line_breaks[inner_line - 1]
		    : 0;
    idx_t end = p.line_breaks[inner_line];
    idx_t line_len = end - start;

    LevelsArray &levels = cache.levels;
    IdxArray &position_L_to_V = cache.position_L_to_V;
    unistring &vis = cache.vis;
    // We apply the BiDi algorithm if the
    // results are not already cached.
    if (!cache.owned_by(cursor.para)) {
	cache.invalidate(); // it's owned by someone else
	levels.resize(p.str.len());
	DBG(100, ("get_embedding_levels() - by calc_vis_column()\n"));
	BiDi::get_embedding_levels(p.str.begin(), p.str.len(),
				   p.base_dir(), levels.begin(),
				   p.breaks_count(), p.line_breaks.begin(),
				   !bidi_enabled);
	vis = p.str;
	position_L_to_V.resize(p.str.len());
	// reorder just the screen line.
	reorder(levels.begin() + start, line_len,
		NULL, position_L_to_V.begin() + start, vis.begin() + start);
    }

    int cursor_log_line_pos = cursor.pos - start;
    // Note that we use the term "width" to make it clear that we
    // sum the widths of the characters. "column", in the name of
    // this method, refers to the terminal column.
    int cursor_vis_width = 0;
    
    if (cursor_log_line_pos >= line_len) {
	// cursor stands at end of line. calculate the
	// width of the whole line.
	if (p.is_rtl())
	    cursor_vis_width = get_rev_str_width(vis.begin() + start,
						 line_len);
	else
	    cursor_vis_width = get_str_width(vis.begin() + start,
					     line_len);
    }
    else {
	// The cursor is inside the line; find the visual position of the
	// cursor, then calculate the width of the segment.
	int cursor_vis_line_pos = position_L_to_V[start + cursor_log_line_pos];
	if (p.is_rtl())
	    cursor_vis_width = get_rev_str_width(
				vis.begin() + start + cursor_vis_line_pos + 1,
				line_len - cursor_vis_line_pos - 1);
	else
	    cursor_vis_width = get_str_width(
				vis.begin() + start,
				cursor_vis_line_pos);
    }

    return cursor_vis_width;
}

// move_to_vis_column() - positions the cursor at a visual column.
// It does a visual-to-logical conversion and then sums the widths of the
// characters, in visual order, till it reachs the right column. then
// it converts this visual column to a logical one.

void EditBox::move_to_vis_column(int column)
{
    Paragraph &p = *curr_para();

    // Find the start and the end of the screen line.
    int inner_line = calc_inner_line();
    idx_t start = (inner_line > 0)
		? p.line_breaks[inner_line - 1]
		: 0;
    idx_t end = p.line_breaks[inner_line];
    idx_t line_len = end - start;

    LevelsArray &levels = cache.levels;
    IdxArray &position_V_to_L = cache.position_V_to_L;
    unistring &vis = cache.vis;
    // We apply the BiDi algorithm if the
    // results are not already cached.
    if (!cache.owned_by(cursor.para)) {
	cache.invalidate(); // it's owned by someone else
	levels.resize(p.str.len());
	DBG(100, ("get_embedding_levels() - by move_to_vis_column()\n"));
	BiDi::get_embedding_levels(p.str.begin(), p.str.len(),
				   p.base_dir(), levels.begin(),
				   p.breaks_count(), p.line_breaks.begin(),
				   !bidi_enabled);
	vis = p.str;
	position_V_to_L.resize(p.str.len());
	// reorder just the screen line.
	reorder(levels.begin() + start, line_len,
		position_V_to_L.begin() + start, NULL, vis.begin() + start);
    }

    if (p.is_rtl()) {
	// revrse the visual string so that we can use the same loop
	// for both RTL and LTR lines.
	reverse(vis.begin() + start, line_len);
    }
	
    // sum the widths of the charaters, in visual order, till we reach
    // the right [visual] column.
    int cursor_vis_line_pos = 0; // silence the compiler
    int width = 0;
    idx_t i;
    wdstate stt;
    for (i = 0; i < line_len; i++) {
	width += get_char_width(vis[start + i], width, &stt, !p.is_rtl());
	if (width > column) {
	    cursor_vis_line_pos = i;
	    break;
	}
    }
    
    if (p.is_rtl()) {
	// cancel the reverse() we did, because the cache is reused latter.
	reverse(vis.begin() + start, line_len);
    }

    // find the logical column corresponding to the visual column.
    int cursor_log_line_pos;

    if (i == line_len) { // cursor at end of line?
	if (inner_line == p.breaks_count() - 1) { // the last inner line?
	    // yes, stand past end of line
	    cursor_log_line_pos = cursor_vis_line_pos = line_len;
	} else {
	    // no, stand on the last char of the line
	    cursor_log_line_pos = cursor_vis_line_pos = line_len - 1;
	}
    } else {
	// use the V_to_L mapping to find the logical column.
	if (p.is_rtl()) {
	    cursor_vis_line_pos = line_len - cursor_vis_line_pos - 1;
	    cursor_log_line_pos = position_V_to_L[start + cursor_vis_line_pos];
	} else {
	    cursor_log_line_pos = position_V_to_L[start + cursor_vis_line_pos];
	}
    }

    cursor.pos = start + cursor_log_line_pos;
}

int EditBox::get_text_width() const
{
    if (!terminal::is_interactive())
	return non_interactive_text_width;
    else
	return window_width() - margin_before - margin_after;
}

// wrap_para() - wraps a paragraph. that means to populate the line_breaks
// array. this method is called when a paragraph has changed or when various
// display options that affect the width of some characters are changed.

void EditBox::wrap_para(Paragraph &para)
{
    para.line_breaks.clear();
  
    if (wrap_type == wrpOff) {
	para.line_breaks.push_back(para.str.len());
	return;
    }
    
    int visible_text_width = get_text_width();
    int line_width = 0;
    idx_t line_len = 0;

    wdstate stt;
    for (idx_t i = 0; i < para.str.len(); i++) {
	int char_width = get_char_width(para.str[i], line_width, &stt);
	if ( (line_width + char_width > visible_text_width && line_len > 0)
		|| para.str[i] == UNICODE_LS )
	{
	    if (para.str[i] == UNICODE_LS) {
		; // do nothing: break after LS; don't trace back to wspace
	    } else if (wrap_type == wrpAtWhiteSpace) {
		// avoid breaking words: break at the previous wspace
		idx_t saved_i = i;
		while (line_len > 0
			    && (!BiDi::is_space(para.str[i])
				|| i == para.str.len() - 1)) {
		    i--;
		    line_len--;
		}
		if (line_len == 0) {
		    // no wspace found; we have to break the word
		    i = saved_i - 1;
		}
	    } else { // wrpAnywhere
		i--;
	    }
	    para.line_breaks.push_back(i + 1);
	    line_len = 0;
	    line_width = 0;
	    stt.clear();
	} else {
	    line_len++;
	    line_width += char_width;
	}
    }

    // add the end-of-paragraph to line_breaks.
    // first make sure it's not already there (e.g. when the paragraph
    // terminates in a LS).
    if (para.line_breaks.empty() || para.line_breaks.back() != para.str.len())
	para.line_breaks.push_back(para.str.len());
}

void EditBox::rewrap_all()
{
    for (int i = 0; i < parags_count(); i++)
	wrap_para(*paragraphs[i]);
    if (wrap_type == wrpOff) {
	// if we've just turned wrap off, make sure the top inner line is 0.
	top_line.inner_line = 0;
    }
    scroll_to_cursor_line();
    request_update(rgnAll);
}

void EditBox::reformat()
{
    rewrap_all();
}

void EditBox::set_wrap_type(WrapType value)
{
    wrap_type = value;
    rewrap_all();
    NOTIFY_CHANGE(wrap);
}

INTERACTIVE void EditBox::toggle_wrap()
{
    switch (wrap_type) {
    case wrpOff:	  set_wrap_type(wrpAtWhiteSpace); break;
    case wrpAtWhiteSpace: set_wrap_type(wrpAnywhere);	  break;
    case wrpAnywhere:	  set_wrap_type(wrpOff);	  break;
    }
}

void EditBox::set_tab_width(int value)
{
    if (value > 0) {
	tab_width = value;
	rewrap_all();
    }
}

void EditBox::resize(int lines, int columns, int y, int x)
{
    Widget::resize(lines, columns, y, x);
    if (old_width != columns) { // has the width changed?
	rewrap_all();
    } else {
	// No, no need to rewrap
	scroll_to_cursor_line();
	request_update(rgnAll);
    }
    old_width = columns;
}

// }}}

// Syntax Highlighting {{{

void EditBox::set_syn_hlt(syn_hlt_t syn)
{
    syn_hlt = syn;
    request_update(rgnAll);
}

INTERACTIVE void EditBox::menu_set_syn_hlt_none()
{
    set_syn_hlt(synhltOff);
}

INTERACTIVE void EditBox::menu_set_syn_hlt_html()
{
    set_syn_hlt(synhltHTML);
}

INTERACTIVE void EditBox::menu_set_syn_hlt_email()
{
    set_syn_hlt(synhltEmail);
}

void EditBox::set_underline(bool v)
{
    underline_hlt = v;
    request_update(rgnAll);
}

INTERACTIVE void EditBox::toggle_underline()
{
    set_underline(!get_underline());
}

// detect_syntax() tries to detect the syntax of the buffer (and turn on
// the appropriate highlighting). If 2 or more lines starting with ">"
// are found within the first 10 lines, it's assumed to be an email
// message. If "<html" or "<HTML" is found within the first 5 lines, it's
// assumed to be HTML.

void EditBox::detect_syntax()
{
    syn_hlt_t syntax = synhltOff;

    int quote_count = 0;
    for (int i = 0; i < 10 && i < parags_count(); i++) {
	const unistring &str = paragraphs[i]->str;
	idx_t len = str.len();
	idx_t pos = 0;
	while (pos < len && str[pos] == ' ')
	    pos++;
	if (pos < len && str[pos] == '>')
	    quote_count++;
    }

    if (quote_count >= 2) {
	syntax = synhltEmail;
    } else {
	for (int i = 0; i < 5 && i < parags_count(); i++) {
	    if ((paragraphs[i]->str.index(u8string("<html")) != -1) ||
		(paragraphs[i]->str.index(u8string("<HTML")) != -1))
	    {
		syntax = synhltHTML;
		break;
	    }
	}
    }

    set_syn_hlt(syntax);
}

// Highlight HTML

static void highlight_html(const unistring &str, EditBox::AttributeArray& attributes)
{
    idx_t len = str.len();
    attribute_t html_attr = get_attr(EDIT_HTML_TAG_ATTR);
    bool is_color = contains_color(html_attr);
    bool in_tag = false;

    for (idx_t pos = 0; pos < len; pos++) {
	if (str[pos] == '<')
	    in_tag = true;
	if (in_tag) {
	    if (is_color)
		attributes[pos] = html_attr;
	    else
		attributes[pos] |= html_attr;
	}
	if (str[pos] == '>')
	    in_tag = false;
    }
}

// Highlight Email

#define MAX_QUOTE_LEVEL 9
static void highlight_email(const unistring &str, EditBox::AttributeArray& attributes)
{
    idx_t len = str.len();
    attribute_t quote_attr = A_NORMAL; // silence the compiler.
    bool is_color = false;
    idx_t pos = 0;
    int level = 0;

    while (pos < len && str[pos] == ' ')
	pos++;
    if (pos < len && str[pos] == '>')
	while (pos < len) {
	    if (str[pos] == '>' && level < MAX_QUOTE_LEVEL) {
		level++;
		quote_attr = get_attr(EDIT_EMAIL_QUOTE1_ATTR + level - 1);
		is_color   = contains_color(quote_attr);
	    }
	    if (is_color)
		attributes[pos] = quote_attr;
	    else
		attributes[pos] |= quote_attr;
	    pos++;
	}
}

inline bool is_ltr_alpha(unichar ch)
{
    return BiDi::is_alnum(ch) && !BiDi::is_rtl(ch);
}

// Highlight underline (*text* and _text_)

static void highlight_underline(const unistring &str, EditBox::AttributeArray& attributes)
{
    idx_t len = str.len();
    attribute_t underline_attr = get_attr(EDIT_EMPHASIZED_ATTR);
    bool is_color = contains_color(underline_attr);
    bool in_emph = false;

    for (idx_t pos = 0; pos < len; pos++) {
	bool emph_ch = (str[pos] == '_' || str[pos] == '*');
	if (emph_ch) {
	    // ignore "http://host/show_bug.cgi" and "ticks_per_second" by making
	    // sure it's not LTR alphanumerics on both sides.
	    if (pos > 0     && is_ltr_alpha(str[pos-1]) &&
		pos < len-1 && is_ltr_alpha(str[pos+1]))
	      emph_ch = false;
	}
	if (emph_ch)
	    in_emph = !in_emph;
	if (in_emph || emph_ch) {
	    if (is_color)
		attributes[pos] = underline_attr;
	    else
		attributes[pos] |= underline_attr;
	}
    }
}

// }}}

// low-level drawing {{{

// get_char_repr() - get representation of some undisplayable characters,
// EOPs, and of some interface elements (e.g. '$').

unichar EditBox::get_char_repr(unichar ch)
{
    if (reprtbl.translate_char(ch)) {
	return ch;
    } else {
	if (reprtbl.empty())
	    return 'X';
	else
	    return ch;
    }
}

// draw_unistr() - draws an LTR visual string.

// latin1_transliterate() - when not in UTF-8 locale, we transliterate
// some Unicode characters to make them readable.

static unichar latin1_transliterate(unichar ch)
{
    switch (ch) {
    // Note: 0xB4 (SPACING ACCENT) and 0xA8 (SPACING DIARESIS) cause
    // some troubles on my Linux console when not in UTF-8 locale, probably
    // because the console driver erroneously thinks it's a non-spacing
    // mark (as U+0301 and U+0308 are).
    case 0xB4:
    case 0xA8:
	    return 'x';

    case 0x203E:
		return 0xAF;	// OVERLINE (we transliterate to "macron" because
				// it's similar)

    case UNI_HEB_MAQAF:
    case UNI_HYPHEN:
    case UNI_NON_BREAKING_HYPHEN:
    case UNI_EN_DASH:
    case UNI_EM_DASH:
    case UNI_MINUS_SIGN:
		 return '-';

    case UNI_HEB_GERESH:
    case UNI_RIGHT_SINGLE_QUOTE:
    case UNI_SINGLE_LOW9_QUOTE:
    case UNI_SINGLE_HIGH_REV9_QUOTE:
		 return '\'';

    case UNI_LEFT_SINGLE_QUOTE:
		return '`';
		 
    case UNI_HEB_GERSHAYIM:
    case UNI_LEFT_DOUBLE_QUOTE:
    case UNI_RIGHT_DOUBLE_QUOTE:
    case UNI_DOUBLE_LOW9_QUOTE:
    case UNI_DOUBLE_HIGH_REV9_QUOTE:
		 return '"';

    case UNI_HEB_SOF_PASUQ:
		 return ':';
    case UNI_HEB_PASEQ:
		 return '|';
    case UNI_BULLET:
		 return '*';
    }
    return ch;
}

void EditBox::draw_unistr(const unichar *str, idx_t len,
			  attribute_t *attributes, int *tab_widths)
{
    unistring buf;
    if (terminal::do_arabic_shaping) {
	buf.append(str, len);
	len = shape(buf.begin(), len, attributes);
	buf.resize(len);
	str = buf.begin();
    }
    
#define SETWATTR(_attr) \
	  if (current_attr != int(_attr)) { \
		wattrset(wnd, _attr); \
		current_attr = _attr; \
	  }

#ifdef HAVE_WIDE_CURSES
#define put_unichar_attr(_wch, _attr) \
	do { \
	    unichar wch = _wch; \
	    if (!terminal::is_utf8) { \
		wch = latin1_transliterate(wch); \
		if (WCTOB(wch) == EOF) { \
		    wch = get_char_repr(FAILED_CONV_REPR); \
		    /*def_attr |= FAILED_CONV_COLOR;*/ \
		    if (contains_color(def_attr)) \
			def_attr = get_attr(EDIT_FAILED_CONV_ATTR); \
		    else \
			def_attr |= get_attr(EDIT_FAILED_CONV_ATTR); \
		} \
	    } \
	    /*SETWATTR(_attr | def_attr);*/ \
	    SETWATTR(contains_color(def_attr) ? def_attr : (_attr | def_attr)); \
	    waddnwstr(wnd, (wchar_t*)&wch, 1); \
	} while (0)
#else
#define put_unichar_attr(_wch, _attr) \
	do { \
	    int ich = latin1_transliterate(_wch); \
	    ich = terminal::force_iso88598 ? unicode_to_iso88598(ich) : WCTOB(ich); \
	    if (ich == EOF) { \
		ich = get_char_repr(FAILED_CONV_REPR); \
		/*def_attr |= FAILED_CONV_COLOR;*/ \
		if (contains_color(def_attr)) \
		    def_attr = get_attr(EDIT_FAILED_CONV_ATTR); \
		else \
		    def_attr |= get_attr(EDIT_FAILED_CONV_ATTR); \
	    } \
	    /*SETWATTR(_attr | def_attr);*/ \
	    SETWATTR(contains_color(def_attr) ? def_attr : (_attr | def_attr)); \
	    waddch(wnd, (unsigned char)ich); \
	} while (0)
#endif

    int tab_counter = 0;
    int line_width = 0;
    int current_attr = -1;

    for (idx_t i = 0; i < len; i++) {
	unichar ch = str[i];
	int char_width = IS_SIMPLE_ONE_COLUMN_CHAR(ch) ? 1 : mk_wcwidth(ch);
	int def_attr = attributes ? attributes[i] : A_NORMAL;

	if (char_width < 1) {
	    if (ch == '\t') {
		if (tab_widths)
		    char_width = tab_widths[tab_counter++];
		else
		    char_width = tab_width - (line_width % tab_width);
		for (int t = 0; t < char_width; t++) {
		    if (show_tabs)
			put_unichar_attr(get_char_repr('\t'), get_attr(EDIT_TAB_ATTR));
		    else
			put_unichar_attr(' ', 0);
		}
	    } else if (char_width == -1 || ch == '\0') {
		// Control characters / nulls: print symbol instead
		char_width = 1;
		put_unichar_attr(get_char_repr(CONTROL_REPR), get_attr(EDIT_CONTROL_ATTR));
	    } else {
		// Non-Spacing Marks.
		// First, handle BIDI explicits and Hebrew NSMs
		if (BiDi::is_explicit_mark(ch)) {
		    if (show_explicits) {
			char_width = 1;
			put_unichar_attr(get_char_repr(ch), get_attr(EDIT_EXPLICIT_ATTR));
		    }
		} else if (BiDi::is_hebrew_nsm(ch) || BiDi::is_arabic_nsm(ch)) {
		    if (rtl_nsm_display == rtlnsmTransliterated) {
			char_width = 1;
			put_unichar_attr(get_char_repr(ch),
				BiDi::is_cantillation_nsm(ch)
				    ? get_attr(EDIT_NSM_CANTILLATION_ATTR)
				    : (BiDi::is_arabic_nsm(ch)
					 ? get_attr(EDIT_NSM_ARABIC_ATTR)
					 : get_attr(EDIT_NSM_HEBREW_ATTR)) );
		    } else if (rtl_nsm_display == rtlnsmAsis
			    && terminal::is_utf8 && !terminal::is_fixed) {
			put_unichar_attr(ch, 0);
		    }
		} else {
		    // Now handle general NSMs
		    if (!terminal::is_utf8 || terminal::is_fixed) {
			char_width = 1;
			put_unichar_attr(get_char_repr(NSM_REPR), get_attr(EDIT_NSM_ATTR));
		    } else {
			// :TODO: new func: is_printable_zw()
			if (ch != UNI_ZWNJ && ch != UNI_ZWJ) {
			    put_unichar_attr(ch, 0);
			}
		    }
		}
	    }
	} else {
	    if (char_width == 2 && (!terminal::is_utf8 || terminal::is_fixed)) {
		// Wide Asian ideograph, but terminal is not capable
		char_width = 1;
		put_unichar_attr(get_char_repr(WIDE_REPR), get_attr(EDIT_WIDE_ATTR));
	    } else {
		attribute_t xattr = 0;
		if (ch == UNICODE_LS) {
		    ch = get_char_repr(UNICODE_LS);
		    xattr = get_attr(EDIT_UNICODE_LS_ATTR);
		} else if (ch == UNI_NO_BREAK_SPACE) {
		    ch = ' ';
		} else if (ch == UNI_HEB_MAQAF) {
		    if (maqaf_display == mqfTransliterated
			    || maqaf_display == mqfHighlighted)
			ch = get_char_repr(UNI_HEB_MAQAF);
		    if (maqaf_display == mqfHighlighted)
			xattr = get_attr(EDIT_MAQAF_ATTR);
		}

		put_unichar_attr(ch, xattr);
	    }
	}
	line_width += char_width;
    }
    SETWATTR(A_NORMAL);
#undef SETWATTR
#undef put_unichar_attr
}

void EditBox::calc_tab_widths(const unichar *str, idx_t len,
			bool rev, IntArray &tab_widths)
{
    tab_widths.clear();
    int line_width = 0;
    wdstate stt;
    idx_t i = rev ? len - 1 : 0;
    while (len--) {
	int char_width = get_char_width(str[i], line_width, &stt);
	if (str[i] == '\t') {
	    if (rev)
		tab_widths.insert(tab_widths.begin(), char_width);
	    else
		tab_widths.push_back(char_width);
	}
	line_width += char_width;
	i += rev ? -1 : 1;
    }
}

// draw_rev_unistr() - draws an RTL visual string. it calls draw_string()
// to do the actuall work. The two functions differ in how they
// calculate TAB widths. draw_rev_unistr() is neccessary because in RTL
// visual strings the first column is at the end of the string. it
// calculates the TAB widths accordingly and passes them to draw_unistring().

void EditBox::draw_rev_unistr(const unichar *str, idx_t len,
			      attribute_t *attributes)
{
    IntArray tab_widths;
    calc_tab_widths(str, len, true, tab_widths);
    draw_unistr(str, len, attributes, tab_widths.begin());
}

void EditBox::put_unichar_attr_at(int line, int col, unichar ch, int attr)
{
    wattrset(wnd, attr);
    wmove(wnd, line, col);
    put_unichar(ch, get_char_repr(FAILED_CONV_REPR));
}

// draw_eop() - draws an EOP symbol.

void EditBox::draw_eop(int y, int x, Paragraph &p, bool selected)
{
    if (!show_paragraph_endings)
	return;

    unichar ch;
    switch (p.eop) {
    case eopMac:    ch = EOP_MAC_REPR; break;
    case eopDOS:    ch = EOP_DOS_REPR; break;
    case eopUnix:   ch = p.is_rtl()
			    ? EOP_UNIX_RTL_REPR
			    : EOP_UNIX_LTR_REPR;
		    break;
    case eopUnicode:
		    ch = EOP_UNICODE_REPR; break;
    default:
		    ch = EOP_NONE_REPR;
    }
    put_unichar_attr_at(y, x, get_char_repr(ch),
			get_attr(EDIT_EOP_ATTR) | (selected ? A_REVERSE : 0));
}

// }}}

// redraw_paragraph {{{

void EditBox::redraw_unwrapped_paragraph(
		    Paragraph &p,
		    int window_start_line,
		    bool only_cursor,
		    int para_num,
		    LevelsArray &levels,
		    IdxArray& position_L_to_V,
		    IdxArray& position_V_to_L,
		    AttributeArray& attributes,
		    bool eop_is_selected
		)
{
    unistring &vis = cache.vis;
    if (!cache.owned_by(para_num)) {
	vis = p.str;
	reorder(levels.begin(), p.str.len(),
		position_V_to_L.begin(),
		position_L_to_V.begin(),
		vis.begin(),
		attributes.begin(), do_mirror,
		(rtl_nsm_display == rtlnsmAsis) && do_mirror/*NSM reordering*/);
    }
    
    // Step 1. find out the start and end of the
    // segment visible on screen.

    // note: we use the term "col", or "pos", to refer to character index, and
    // "width" to refer to the visual position on screen -- sum of the widths
    // of the preceeding character.

    // find the visual cursor index
    int	    cursor_vis_width = 0;
    idx_t   cursor_vis_line_pos = -1;

    if (curr_para() == &p) {
	if (cursor.pos == p.str.len())
	    cursor_vis_line_pos = p.str.len();
	else {
	    cursor_vis_line_pos = position_L_to_V[cursor.pos];
	    if (p.is_rtl())
		cursor_vis_line_pos = p.str.len() - cursor_vis_line_pos - 1;
	}
    }

    idx_t start_col = 0, end_col = 0;
    int visible_text_width = get_text_width();
    int segment_width = 0;
    
    if (p.is_rtl()) {
	reverse(vis.begin(), p.str.len());
	reverse(attributes.begin(), p.str.len());
    }

    idx_t i;
    wdstate stt;
    for (i = 0; i < p.str.len(); i++) {
	int char_width = get_char_width(vis[i], segment_width, &stt, !p.is_rtl());
	segment_width += char_width;

	if (segment_width > visible_text_width) {
	    if (cursor_vis_line_pos < i) {
		// we've found the end of the segment.
		segment_width -= char_width;
		end_col = i;
		break;
	    } else {
		// start a new segment. recalculate the width of this
		// first character because TAB's width may change.
		stt.clear();
		char_width = get_char_width(vis[i], 0, &stt, !p.is_rtl());
		segment_width = char_width;
		start_col = i;
	    }
	}
	if (i == cursor_vis_line_pos)
	    cursor_vis_width = segment_width - char_width;
    }
    if (end_col == 0)
	end_col = i;
    if (cursor.pos == p.str.len()) {
	cursor_vis_width = segment_width;
    }

    if (p.is_rtl()) {
	reverse(vis.begin() + start_col, end_col - start_col);
	reverse(attributes.begin() + start_col, end_col - start_col);
    }
    
    // Step 2. draw the segment [start_col .. end_col)

    if (p.is_rtl()) {
	wmove(wnd, window_start_line,
	      margin_after + visible_text_width - segment_width);
	draw_rev_unistr(vis.begin() + start_col, end_col - start_col,
			attributes.begin() + start_col);
    } else {
	wmove(wnd, window_start_line, margin_before);
	draw_unistr(vis.begin() + start_col, end_col - start_col,
		    attributes.begin() + start_col);
    }

    if (p.is_rtl()) {
	// cancel the reverse() we did, because the cache is reused latter.
	reverse(vis.begin() + start_col, end_col - start_col);
	reverse(vis.begin(), p.str.len());
    }

    // Step 3. draw EOP / continuation indicator
    if (end_col != p.str.len()) {
	// if the end of the para is not shown,
	// draw line-countinuation indicator ('$')
	put_unichar_attr_at(
		 window_start_line,
		 p.is_rtl() ? margin_after - 1
			    : margin_before + visible_text_width,
		 get_char_repr(TRIM_REPR), get_attr(EDIT_TRIM_ATTR));
    } else {
	// if the end is shown, draw EOP
	draw_eop(window_start_line,
		 p.is_rtl() ? margin_after + visible_text_width
				- segment_width - 1
			    : margin_before + segment_width,
		 p, eop_is_selected);
    }
    
    if (start_col != 0 && cursor_vis_width > 2) {
	// if the beginning of the para is not shown,
	// draw another line-continuation indicator, at the other side.
	put_unichar_attr_at(
		 window_start_line, 
		 p.is_rtl() ? margin_after + visible_text_width - 1
			    : margin_before,
		 get_char_repr(TRIM_REPR), get_attr(EDIT_TRIM_ATTR));
    }

    // Step 4. position the cursor

    if (p.is_rtl()) {
	cursor_vis_width = visible_text_width - cursor_vis_width - 1;
	wmove(wnd, window_start_line, margin_after + cursor_vis_width);
    } else {	
	wmove(wnd, window_start_line, margin_before + cursor_vis_width);
    }
}

void EditBox::redraw_wrapped_paragraph(
		    Paragraph &p,
		    int window_start_line,
		    bool only_cursor,
		    int para_num,
		    LevelsArray &levels,
		    IdxArray& position_L_to_V,
		    IdxArray& position_V_to_L,
		    AttributeArray& attributes,
		    bool eop_is_selected
		)
{
    idx_t cursor_vis_line_pos = -1;
    int   cursor_vis_width = 0;
    int   cursor_line = -1;

    unistring &vis = cache.vis;
    if (!cache.owned_by(para_num))
	vis = p.str;
    
    int visible_text_width = get_text_width();
    idx_t prev_line_break = 0;

    // draw the paragraph line by line.
    for (int line_num = 0;
	     line_num < p.breaks_count();
	     line_num++)
    {
	idx_t line_break = p.line_breaks[line_num];
	idx_t line_len = line_break - prev_line_break;
	bool  is_last_line = (line_num == p.breaks_count() - 1);

	if (!cache.owned_by(para_num)) {
	    reorder(levels.begin() + prev_line_break, line_len,
		    position_V_to_L.begin() + prev_line_break,
		    position_L_to_V.begin() + prev_line_break,
		    vis.begin() + prev_line_break,
		    attributes.begin() + prev_line_break, do_mirror,
		    (rtl_nsm_display == rtlnsmAsis) && do_mirror/*NSM reordering*/);
	}

	// draw segment [prev_line_break .. line_break)
	if ( !only_cursor && window_start_line + line_num >= 0
		&& window_start_line + line_num < window_height() ) {
	    // :TODO: handle TABS at end of line (especially RTL lines)
	    if (p.is_rtl()) {
		int line_width = get_rev_str_width(
				    vis.begin() + prev_line_break, line_len);
		int window_x = margin_after;
		// we reserve one space for wrap (margin_after), so:
		if (line_width > visible_text_width)
		    window_x--;
		else
		    window_x += visible_text_width - line_width;
		wmove(wnd, window_start_line + line_num, window_x);
		draw_rev_unistr(vis.begin() + prev_line_break, line_len,
				attributes.begin() + prev_line_break);
		if (is_last_line) {
		    draw_eop(window_start_line + line_num, window_x - 1,
			     p, eop_is_selected);
		} else if (wrap_type == wrpAnywhere) {
		    put_unichar_attr_at(
			     window_start_line + line_num,
			     margin_after - 1,
			     get_char_repr(WRAP_RTL_REPR), get_attr(EDIT_WRAP_ATTR));
		}
	    } else {
		wmove(wnd, window_start_line + line_num, margin_before);
		draw_unistr(vis.begin() + prev_line_break, line_len,
			    attributes.begin() + prev_line_break);
		if (is_last_line) {
		    draw_eop(window_start_line + line_num,
			     margin_before + get_str_width(vis.begin()
						+ prev_line_break, line_len),
			     p, eop_is_selected);
		} else if (wrap_type == wrpAnywhere) {
		    put_unichar_attr_at(
			     window_start_line + line_num,
			     margin_before + visible_text_width,
			     get_char_repr(WRAP_LTR_REPR), get_attr(EDIT_WRAP_ATTR));
		}
	    }
	}

	// find the visual cursor position
	if (curr_para() == &p && cursor_line == -1) {
	    if (cursor.pos < line_break) {
		int cursor_log_line_pos = cursor.pos - prev_line_break;
		cursor_vis_line_pos = position_L_to_V[
				prev_line_break + cursor_log_line_pos];
		if (p.is_rtl()) {
		    cursor_vis_width = get_rev_str_width(
					vis.begin() + prev_line_break
					    + cursor_vis_line_pos + 1,
					line_len - cursor_vis_line_pos - 1);
		} else {
		    cursor_vis_width = get_str_width(
					vis.begin() + prev_line_break,
					cursor_vis_line_pos);
		}
		cursor_line = line_num;
	    }
	    else if (cursor.pos == p.str.len() && is_last_line) {
		if (p.is_rtl()) {
		    cursor_vis_width = get_rev_str_width(
					vis.begin() + prev_line_break,
					line_len);
		} else {
		    cursor_vis_width = get_str_width(
					vis.begin() + prev_line_break,
					line_len);
		}
		cursor_line = line_num;
	    }
	}

	prev_line_break = line_break;
    }

    // position the cursor
    if (cursor_line != -1) {
        if (p.is_rtl()) {
	    cursor_vis_width = visible_text_width - cursor_vis_width - 1;
	    wmove(wnd,
		  window_start_line + cursor_line,
		  margin_after + cursor_vis_width);
	} else {
	    wmove(wnd,
		  window_start_line + cursor_line,
		  margin_before + cursor_vis_width);
	}
    }
}

void EditBox::do_syntax_highlight(const unistring &str, 
	    AttributeArray &attributes, int para_num)
{
    switch (syn_hlt) {
    case synhltHTML:
	highlight_html(str, attributes);
	break;
    case synhltEmail:
	highlight_email(str, attributes);
	break;
    default:
	break;
    }
    if (underline_hlt)
	highlight_underline(str, attributes);
}
	
void EditBox::redraw_paragraph(Paragraph &p, int window_start_line,
			       bool only_cursor, int para_num)
{
    //if (is_primary_mark_set()
	//cache.invalidate();

    if (is_primary_mark_set()
	    || wrap_type == wrpOff) // FIXME: when wrap=off, highlighting fucks up in RTL lines;
				    //        cache.invalidate() is a temporary solution.
	cache.invalidate();
   
    LevelsArray &levels = cache.levels;
    IdxArray &position_L_to_V = cache.position_L_to_V;
    IdxArray &position_V_to_L = cache.position_V_to_L;
    AttributeArray &attributes = cache.attributes; 

    if (!cache.owned_by(para_num)) {
	position_L_to_V.resize(p.str.len());
	position_V_to_L.resize(p.str.len());
	levels.resize(p.str.len());
	attributes.clear();
	attributes.resize(p.str.len());
	DBG(100, ("get_embedding_levels() - by redraw_paragraph()\n"));
	BiDi::get_embedding_levels(p.str.begin(), p.str.len(),
				   p.base_dir(), levels.begin(),
				   p.breaks_count(), p.line_breaks.begin(),
				   !bidi_enabled);
	do_syntax_highlight(p.str, attributes, para_num);
    }

    //AttributeArray attributes(p.str.len()); 
    bool eop_is_selected = false;

    // We use the "attributes" array to highlight the selection.
    // for each selected characetr, we "OR" the corresponding
    // attribute element with A_REVERSE.
    if (is_primary_mark_set()) {
	Point lo = primary_mark, hi = cursor;
	if (hi < lo)
	    hi.swap(lo);
	if (lo.para <= para_num && hi.para >= para_num) {
	    idx_t start, end;
	    if (lo.para == para_num)
		start = lo.pos;
	    else
		start = 0;
	    if (hi.para == para_num)
		end = hi.pos;
	    else {
		eop_is_selected = true;
		end = p.str.len();
	    }

	    attribute_t selected_attr = get_attr(EDIT_SELECTED_ATTR);
	    idx_t i;
	    if (contains_color(selected_attr))
		for (i = start; i < end; i++)
		    attributes[i] = selected_attr;
	    else
		for (i = start; i < end; i++)
		    attributes[i] |= selected_attr;
	}
    }

    if (wrap_type == wrpOff) {
	redraw_unwrapped_paragraph(
		    p,
		    window_start_line,
		    only_cursor,
		    para_num,
		    levels,
		    position_L_to_V,
		    position_V_to_L,
		    attributes,
		    eop_is_selected
		);
    } else {
	redraw_wrapped_paragraph(
		    p,
		    window_start_line,
		    only_cursor,
		    para_num,		    
		    levels,
		    position_L_to_V,
		    position_V_to_L,
		    attributes,
		    eop_is_selected
		);
    }

    cache.set_owner(para_num);
}

// }}}

// Log2Vis {{{

// emph_string() - emphasize marked-up segments. I.e. converts
// "the *quick* fox" to "the q_u_i_c_k_ fox". but "* bullet"
// stays "* bullet".

static void emph_string(unistring &str,
			unichar emph_marker, unichar emph_ch)
{
    bool in_emph = false;
    for (int i = 0; i < str.len(); i++) {
	if ( (!emph_marker && (str[i] == '*' || str[i] == '_')) ||
	     ( emph_marker &&  str[i] == emph_marker ) ) {
	    if (!(!in_emph &&
		  i < str.len()-1 &&
		  BiDi::is_space(str[i+1])) )
	    {
		in_emph = !in_emph;
		str.erase(str.begin() + i);
		i--;
	    }
	} else {
	    if (in_emph) {
		i++;
		// skip all NSMs.
		while (i < str.len() && BiDi::is_nsm(str[i]))
		    i++;
		str.insert(str.begin()+i, emph_ch);
	    }
	}
    }
}

// log2vis() - convert the [logical] document into visual.

void EditBox::log2vis(const char *options)
{
    bool    opt_bdo	    = false;
    bool    opt_nopad       = false;
    bool    opt_engpad	    = false;

    unichar opt_emph	    = false;
    unichar opt_emph_marker = 0;
    unichar opt_emph_ch     = UNI_NS_UNDERSCORE;

    // parse the 'options' string.
    if (options) {
	if (strstr(options, "bdo"))
	    opt_bdo = true;
	if (strstr(options, "nopad"))
	    opt_nopad = true;
	if (strstr(options, "engpad"))
	    opt_engpad = true;
	const char *s;
	if ((s = strstr(options, "emph"))) {
	    opt_emph = true;
	    if (s[4] == ':') {
		char *endptr;
		opt_emph_ch = strtol(s + 5, &endptr, 0);
		s = endptr;
		if (s && (*s == ':' || *s == ','))
		    opt_emph_marker = strtol(s + 1, NULL, 0);
	    }
	    DBG(1, ("--EMPH-- glyph: U+%04lX, marker: U+%04lX\n",
				    (unsigned long)opt_emph_ch,
				    (unsigned long)opt_emph_marker));
	}
    }
   
    std::vector<unistring> visuals;
    
    for (int i = 0; i < parags_count(); i++)
    {
	Paragraph &p = *paragraphs[i];

	unistring &visp = p.str;
	if (opt_emph) {
	    emph_string(visp, opt_emph_marker, opt_emph_ch);
	    post_para_modification(p);
	}
	
	cache.invalidate();
	LevelsArray &levels = cache.levels;
	IdxArray &position_L_to_V = cache.position_L_to_V;
	IdxArray &position_V_to_L = cache.position_V_to_L;

	position_L_to_V.resize(p.str.len());
	position_V_to_L.resize(p.str.len());
	levels.resize(p.str.len());
	DBG(100, ("get_embedding_levels() - by log2vis()\n"));
	BiDi::get_embedding_levels(p.str.begin(), p.str.len(),
				   p.base_dir(), levels.begin(),
				   p.breaks_count(), p.line_breaks.begin());

	int visible_text_width = get_text_width();
	idx_t prev_line_break = 0;

	for (int line_num = 0;
		 line_num < p.breaks_count();
		 line_num++)
	{
	    idx_t line_break = p.line_breaks[line_num];
	    idx_t line_len = line_break - prev_line_break;
	   
	    // trim
	    while (line_len
		    && BiDi::is_space(p.str[prev_line_break + line_len - 1]))
	      line_len--;

	    // convert to visual
	    reorder(levels.begin() + prev_line_break, line_len,
		    position_V_to_L.begin() + prev_line_break,
		    position_L_to_V.begin() + prev_line_break,
		    visp.begin() + prev_line_break,
		    NULL, true,
		    rtl_nsm_display == rtlnsmAsis);

	    if (terminal::do_arabic_shaping)
		line_len = shape(p.str.begin() + prev_line_break,
				 line_len, NULL);

	    unistring visline;
	    // pad RTL lines
	    if (p.is_rtl() && !opt_nopad) {
		int line_width = get_rev_str_width(
				    visp.begin() + prev_line_break, line_len);
		while (line_width < visible_text_width) {
		    visline.push_back(' ');
		    line_width++;
		}
	    }

	    // convert TABs to spaces, and convert/erase some
	    // special chars.
	    IntArray tab_widths;
	    int tab_counter = 0;
	    calc_tab_widths(visp.begin() + prev_line_break, line_len,
			    p.is_rtl(), tab_widths);
	    for (int i = prev_line_break; i < prev_line_break + line_len; i++) {
		if (visp[i] == '\t') {
		    int j = tab_widths[tab_counter++];
		    while (j--)
			visline.push_back(' ');
		} else if (visp[i] == UNI_HEB_MAQAF
			    && maqaf_display != mqfAsis) {
		    visline.push_back('-');
		} else {
		    if (!BiDi::is_transparent_formatting_code(visp[i]))
			visline.push_back(visp[i]);
		}
	    }

	    // pad LTR lines
	    if (!p.is_rtl() && opt_engpad) {
		int line_width = get_str_width(
				    visp.begin() + prev_line_break, line_len);
		while (line_width < visible_text_width) {
		    visline.push_back(' ');
		    line_width++;
		}
	    }
	    
	    if (opt_bdo) {
		visline.insert(visline.begin(), UNI_LRO);
		visline.push_back(UNI_PDF);
	    }
	    visuals.push_back(visline);
	    
	    prev_line_break = line_break;
	}

	delete paragraphs[i];
    }

    // replace the logical document with the visual version.
    paragraphs.clear();
    for (int i = 0; i < (int)visuals.size(); i++) {
	Paragraph *p = new Paragraph();
	p->str = visuals[i];
	p->eop = eopUnix;
	post_para_modification(*p);
	// skip the last empty line.
	if (!(i == (int)visuals.size() - 1 && p->str.len() == 0))
	    paragraphs.push_back(p);
    }

    undo_stack.clear();
    unset_primary_mark();
    set_modified(true);
    cursor.zero();
    scroll_to_cursor_line();
    request_update(rgnAll);
}

// }}}

