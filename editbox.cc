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

#include <algorithm>

#include "editbox.h"
#include "scrollbar.h"
#include "transtbl.h"
#include "univalues.h"
#include "themes.h"
#include "dbg.h"

TranslationTable EditBox::transtbl;
TranslationTable EditBox::reprtbl;
TranslationTable EditBox::altkbdtbl;

// DOS's EOP is actually two characters: CR + LF, but we represent it
// internally as one character to make processing much simpler. The value we
// choose for it is in Unicode's Private Area block.

#define DOS_PS	    0xF880
#define MAC_PS	    0x0D
#define UNIX_PS	    0x0A

// Initialization {{{

EditBox::EditBox()
{
    create_window();

    status_listener	= NULL;
    error_listener	= NULL;
    margin_before	= 0;
    margin_after	= 1;
    primary_mark.para	= -1;
    optimal_vis_column	= -1;
    bidi_enabled	= true;
    visual_cursor_movement = false;
    modified		= false;
    scroll_step		= 4;
    update_region	= rgnAll;
    alt_kbd		= false;
    auto_justify	= false;
    justification_column = 72;
    auto_indent		= false;
    translation_mode	= false;
    read_only		= false;
    smart_typing	= false;
    wrap_type		= wrpAtWhiteSpace;
    dir_algo		= algoContextRTL;
    tab_width		= 8;
    show_tabs		= true;
    show_explicits	= true;
    show_paragraph_endings = true;
    rtl_nsm_display	= rtlnsmTransliterated;
    maqaf_display	= mqfAsis;
    syn_hlt		= synhltOff;
    underline_hlt	= false;
    rfc2646_trailing_space = true;
    data_transfer.in_transfer = false;
    old_width		= -1;
    prev_command_type = current_command_type = cmdtpUnknown;
    paragraphs.push_back(new Paragraph());
}

EditBox::~EditBox()
{
    for (int i = 0; i < parags_count(); i++)
	delete paragraphs[i];
}
  
void EditBox::new_document()
{
    for (int i = 0; i < parags_count(); i++)
	delete paragraphs[i];
    paragraphs.clear();
    paragraphs.push_back(new Paragraph());

    undo_stack.clear();
    unset_primary_mark();
    set_modified(false);
    cursor.zero();
    scroll_to_cursor_line();
    request_update(rgnAll);
}

void EditBox::sync_scrollbar(Scrollbar *scrollbar)
{
    scrollbar->set_total_size(parags_count());
    scrollbar->set_page_size(window_height());
    scrollbar->set_page_pos(top_line.para);
}

// }}}

// Utility methods dealing with End-Of-Paragraphs {{{

static inline bool is_eop(unichar ch) {
    return (ch == DOS_PS || ch == MAC_PS || ch == UNIX_PS || ch == UNICODE_PS);
}

static inline eop_t get_eop_type(unichar ch)
{
    switch (ch) {
    case DOS_PS:     return eopDOS;
    case MAC_PS:     return eopMac;
    case UNIX_PS:    return eopUnix;
    case UNICODE_PS: return eopUnicode;
    default:	     return eopNone;
    }
}

unichar EditBox::get_eop_char(eop_t eop)
{
    switch (eop) {
    case eopDOS:	return DOS_PS;
    case eopMac:	return MAC_PS;
    case eopUnix:	return UNIX_PS;
    case eopUnicode:	return UNICODE_PS;
    default:		return UNIX_PS; // we shouldn't arrive here
    }
}

// get_curr_eop_char() returns the character corresponding to the current
// paragraph's EOP. 
//
// When creating new paragraphs, e.g. as a reslt of pressing ENTER or of
// justifying lines, we copy the EOP of the current paragraph to the new
// paragraphs.  Since our document may contain differenet EOPs (for different
// paragraphs), this seems to be the most sensible solution.
//
// When the current paragraph has no EOP (when it's the last in the buffer),
// we look at the previous one. As a last resort we use Unix's EOP.

unichar EditBox::get_curr_eop_char()
{
    if (curr_para()->eop == eopNone) {
	if (cursor.para > 0)
	    return get_eop_char(paragraphs[cursor.para - 1]->eop);
	else
	    return UNIX_PS; // default
    } else {
	return get_eop_char(curr_para()->eop);
    }
}

// toggle_eops() is an interactive command to change all EOPs in the buffer.
// 
// It'd be especially appreciated by Unix users who would like to convert
// DOS line ends to Unix line ends, but it was written to toggle among all
// possible EOPs.

void EditBox::set_eops(eop_t new_eop)
{
    if (read_only) {
	NOTIFY_ERROR(read_only);
	return;
    }
    
    for (int i = 0; i < parags_count() - 1; i++)
	paragraphs[i]->eop = new_eop;
    set_modified(true);
    
    request_update(rgnAll);
}

INTERACTIVE void EditBox::toggle_eops()
{
    eop_t new_eop = eopNone; // silence the compiler
    switch (paragraphs[0]->eop) {
    case eopUnix:	new_eop = eopDOS;	break;
    case eopDOS:	new_eop = eopMac;	break;
    case eopMac:	new_eop = eopUnicode;	break;
    case eopUnicode:	new_eop = eopUnix;	break;
    case eopNone:	return; // no eops in document
    }
    set_eops(new_eop);
}

// }}}

// Setters / Getters / Togglers {{{

// The following are trivial set_xxx methods.

void EditBox::set_read_only(bool value) {
    read_only = value;
    NOTIFY_CHANGE(read_only);
}

void EditBox::set_smart_typing(bool val) {
    smart_typing = val;
    NOTIFY_CHANGE(smart_typing);
}

void EditBox::set_modified(bool value) {
    modified = value;
    NOTIFY_CHANGE(modification);
}

void EditBox::set_auto_indent(bool value) {
    auto_indent = value;
    NOTIFY_CHANGE(auto_indent);
}

void EditBox::set_auto_justify(bool value) {
    auto_justify = value;
    NOTIFY_CHANGE(auto_justify);
}

void EditBox::set_justification_column(int value) {
    if (value > 0)
	justification_column = value;
}

void EditBox::set_scroll_step(int value) {
    if (value > 0)
	scroll_step = value;
}

void EditBox::set_primary_mark(const Point &point) {
    primary_mark = point;
    NOTIFY_CHANGE(selection);
}

void EditBox::unset_primary_mark() {
    if (is_primary_mark_set()) {
	primary_mark.para = -1;
	request_update(rgnAll);
	NOTIFY_CHANGE(selection);
    }
}

INTERACTIVE void EditBox::toggle_primary_mark() {
    if (is_primary_mark_set())
	unset_primary_mark();
    else
	set_primary_mark();
}

// }}}

// Misc {{{

// calc_distance() - count the number of characters, counting EOP as 1,
// between two points.

int EditBox::calc_distance(Point p1, Point p2)
{
    int dist;
    if (p2 < p1)
	p2.swap(p1);

    if (p1.para == p2.para) {
	dist = p2.pos - p1.pos;
    } else {
	dist = paragraphs[p1.para]->str.len() - p1.pos + 1;
	while (++p1.para < p2.para)
	    dist += paragraphs[p1.para]->str.len() + 1;
	dist += p2.pos;
    }
    return dist;
}

// calc_inner_line() returns the inner-line on which the cursor stands.
// In our terminology, "inner-line" is a paragraph screen-line, zero-based.

int EditBox::calc_inner_line()
{
    for (int line_num = 0; line_num < curr_para()->breaks_count(); line_num++) {
	idx_t line_break = curr_para()->line_breaks[line_num];
	if (cursor.pos < line_break
	    || (cursor.pos == line_break
		&& cursor.pos == curr_para()->str.len()))
	    return line_num;
    }
    return 0;
}

// }}}

// Clipboard {{{

INTERACTIVE void EditBox::copy() {
    cut_or_copy(true);
}

INTERACTIVE void EditBox::cut() {
    cut_or_copy(false);
}

INTERACTIVE void EditBox::paste() {
    insert_text(clipboard);
}

// cut_or_copy() -- cut and/or copies text into the clipboard. It does it by
// calling copy_text() and delete_text().
//
// @param just_copy - don't delete the text.

void EditBox::cut_or_copy(bool just_copy)
{
    if (is_primary_mark_set()) {
	int len = calc_distance(cursor, primary_mark);
	// copy_text() and delete_text() advance the cursor --
	// that's why this code is a bit confusing: we restore the
	// cursor position after each call.
	Point orig_cursor = cursor;
	if (cursor > primary_mark)
	    cursor = primary_mark;
	if (just_copy) {
	    copy_text(len);
	    cursor = orig_cursor;
	} else {
	    Point tmp = cursor;
	    copy_text(len);
	    cursor = tmp;
	    delete_text(len);
	}
	unset_primary_mark();
    } else {
	NOTIFY_ERROR(no_selection);
    }
}

// copy_text() -- copies "len" characters to the clipboard. advances the
// cursor.
// if the 'append' parameter is true, the characters don't replace the
// clipboard but are appended to it.

void EditBox::copy_text(int len, bool append)
{
    if (!append)
	clipboard.clear();
    while (len > 0) {
	if (cursor.pos == curr_para()->str.len()) {
	    if (cursor.para < parags_count() - 1) {
		clipboard.push_back(get_curr_eop_char());
		cursor.para++;
		cursor.pos = 0;
	    }
	    len--;
	} else {
	    int to_copy = MIN(curr_para()->str.len() - cursor.pos, len);
	    clipboard.append(curr_para()->str.begin() + cursor.pos, to_copy);
	    cursor.pos += to_copy;
	    len -= to_copy;
	}
    }
}

// }}}

// Undo {{{

INTERACTIVE void EditBox::undo()
{
    undo_op(undo_stack.get_prev_op());
    // If we undo all the changes to the buffer, then we return it to its
    // initial state. clear the "modified" flag.
    if (!undo_stack.is_undo_available()
	    && !undo_stack.disabled() && !undo_stack.was_truncated())
	set_modified(false);
}

INTERACTIVE void EditBox::redo()
{
    redo_op(undo_stack.get_next_op());
}

// undo_op() -- undoes en operation. For example, if the operation was to
// delete some text then we re-insert the deleted text.
//
// There are three fundamental operations: inserting text, deleting text and
// replacing text.

void EditBox::undo_op(UndoOp *op)
{
    if (!op)
	return;

    switch (op->type) {
    case opDelete:
	cursor = op->point;
	insert_text(op->deleted_text, true);
	break;
    case opInsert:
	cursor = op->point;
	delete_text(op->inserted_text.len(), true);
	break;
    case opReplace:
	cursor = op->point;
	replace_text(op->deleted_text, op->inserted_text.len(), true);
	break;
    }
}
    
void EditBox::redo_op(UndoOp *op)
{
    if (!op)
	return;
    
    switch (op->type) {
    case opDelete:
	cursor = op->point;
	delete_text(op->deleted_text.len(), true);
	break;
    case opInsert:
	cursor = op->point;
	insert_text(op->inserted_text, true);
	break;
    case opReplace:
	cursor = op->point;
	replace_text(op->inserted_text, op->deleted_text.len(), true);
	break;
    }
}

INTERACTIVE void EditBox::toggle_key_for_key_undo()
{
    set_key_for_key_undo(!is_key_for_key_undo());
}

// }}}

// Data Transfer {{{

// start_data_transfer() - must be called to start any data transfer; it
// initializes the data tranfser state (the "data_transfer" struct).
//
// @param dir - the direction in which the data is transferred. Either into
// EditBox (dataTransferIn) or out of EditBox (dataTransferOut).
// 
// @param new_doc - clear the buffer before transferring data into EditBox.
// 
// @param selection_only - when transferring data out, transfer only the
// selection, not the whole buffer.

void EditBox::start_data_transfer(data_transfer_direction dir,
				  bool new_doc,
				  bool selection_only)
{
    data_transfer.dir = dir;
    data_transfer.in_transfer = true;

    if (dir == dataTransferIn) {
	// Loading a file
	if (new_doc) {
	    // "Open new file"
	    new_document();
	    data_transfer.skip_undo = true;
	    data_transfer.clear_modified_flag = true;
	} else {
	    // "Insert file"
	    data_transfer.skip_undo = false;
	    data_transfer.clear_modified_flag = false;
	}
	data_transfer.prev_is_cr = false;
	data_transfer.cursor_origin = cursor;
    } else {
	// Saving a file
	data_transfer.cursor_origin = cursor;
	if (selection_only && is_primary_mark_set()) {
	    data_transfer.ntransferred_out = 0;
	    data_transfer.ntransferred_out_max = calc_distance(cursor, primary_mark);
	    if (cursor > primary_mark)
		cursor = primary_mark;
	} else {
	    data_transfer.ntransferred_out_max = -1;
	    cursor.zero();
	}
	data_transfer.at_eof = false;
	// An error may occur during saving, so only a an object that knows
	// the IO result may clear the modification flag.
	data_transfer.clear_modified_flag = false;
    }
}

bool EditBox::is_in_data_transfer() const {
    return data_transfer.in_transfer;
}

int EditBox::transfer_data(unichar *buf, int len)
{
    if (data_transfer.dir == dataTransferIn)
	return transfer_data_in(buf, len);
    else
	return transfer_data_out(buf, len);
}

// transfer_data_out() - transfers data out of EditBox. returns the number of
// characters placed in "buf". when it returns 0, we know we've reached end
// of buffer.

int EditBox::transfer_data_out(unichar *buf, int len)
{
    if (data_transfer.at_eof)
	return 0;	// end-of-buffer: no more chars to write

    int nwritten = 0;	// number of chars we've written to buf
    while (nwritten < len && !data_transfer.at_eof) {
	if (cursor.pos == curr_para()->str.len()) {

	    // write EOP
	    if (curr_para()->eop != eopNone) {
		if (curr_para()->eop == eopDOS) {
		    // DOS's EOP is two characters. If there's not
		    // enough space in buf, we exit and do it in the
		    // next call.
		    if (len - nwritten < 2)
			break;
		    *buf++ = '\r';
		    *buf++ = '\n';
		    nwritten += 2;
		} else {
		    *buf++ = get_curr_eop_char();
		    nwritten++;
		}
		data_transfer.ntransferred_out++;
		if (data_transfer.ntransferred_out_max != -1
			&& data_transfer.ntransferred_out
				>= data_transfer.ntransferred_out_max)
		    data_transfer.at_eof = true;
	    }
	    
	    if (cursor.para < parags_count() - 1) {
		// advance to next paragraph
		cursor.para++;
		cursor.pos = 0;
	    } else {
		// we've reached end of buffer
		data_transfer.at_eof = true;
	    }
	} else {
	    // write the [cursor, end-of-paragraph) segment into buf.
	    int to_copy = MIN(curr_para()->str.len() - cursor.pos, (len - nwritten));
	    if (data_transfer.ntransferred_out_max != -1
		    && data_transfer.ntransferred_out + to_copy
			    > data_transfer.ntransferred_out_max) {
		to_copy = MAX(data_transfer.ntransferred_out_max
				- data_transfer.ntransferred_out, 0);
	    }
	    unichar *src = curr_para()->str.begin() + cursor.pos;
	    cursor.pos += to_copy;
	    nwritten += to_copy;
	    data_transfer.ntransferred_out += to_copy;
	    while (to_copy--)
		*buf++ = *src++;
	    if (data_transfer.ntransferred_out_max != -1
		    && data_transfer.ntransferred_out
			    >= data_transfer.ntransferred_out_max)
		data_transfer.at_eof = true;
	}
    }
    return nwritten;
}

#define INSERT_DOS_PS() \
    do { \
	unichar ch = DOS_PS; \
	insert_text(&ch, 1, data_transfer.skip_undo); \
    } while (0)
#define INSERT_CR() \
    do { \
	unichar ch = '\r'; \
	insert_text(&ch, 1, data_transfer.skip_undo); \
    } while (0)

// transfer_data_in() - transfers data into EditBox.

int EditBox::transfer_data_in(unichar *data, int len)
{
    // What makes the implementation complicated is that we need to convert
    // DOS's EOP (CR + LF) to one pseudo character. Since these two characters
    // may be passed to this method in two separate calls, we have to maintain
    // state (prev_is_cr = "last character was CR").
    int start = 0;
    if (data_transfer.prev_is_cr && len) {
	if (data[0] == '\n') {
	    INSERT_DOS_PS();
	    start = 1;
	} else {
	    INSERT_CR();
	}
	data_transfer.prev_is_cr = false;
    }
    for (int i = start; i < len; i++) {
	if (data[i] == '\r') {
	    insert_text(data + start, i - start, data_transfer.skip_undo);
	    start = i;
	    if (i + 1 < len) {
		if (data[i + 1] == '\n') {
		    INSERT_DOS_PS();
		    start += 2;
		    i++;
		}
	    } else {
		data_transfer.prev_is_cr = true;
		start++;
	    }
	}
    }
    insert_text(data + start, len - start, data_transfer.skip_undo);
    return 0; // return value is meaningless
}

// end_data_transfer() -- must be called when transfer is completed.

void EditBox::end_data_transfer()
{
    if (data_transfer.dir == dataTransferIn) {
	if (data_transfer.prev_is_cr)
	    INSERT_CR();
    }

    if (data_transfer.clear_modified_flag)
	set_modified(false);
    data_transfer.in_transfer = false;

    cursor = data_transfer.cursor_origin;
    if (data_transfer.dir == dataTransferIn)
	last_modification = cursor;
    scroll_to_cursor_line();
    request_update(rgnAll);
}

#undef INSERT_DOS_PS
#undef INSERT_CR

// }}}

// Scrolling {{{

// add_rows_to_line() - adds screen lines to a CombinedLine. For example, if
// we advance the cursor forward by one screen line, we could find the new
// line using:
//
// CombinedLine line(cursor.para, calc_inner_line());
// add_rows_to_line(line, 1);
//
// @params rows - screen rows to add. can be negative.

void EditBox::add_rows_to_line(CombinedLine &combline, int rows)
{
    if (rows > 0) {
	int remaining_line_breaks = paragraphs[combline.para]->breaks_count()
				    - combline.inner_line - 1;
	if (rows <= remaining_line_breaks) {
	    // Our final position is inside the paragraph.
	    // We zero rows to finish the loop.
	    combline.inner_line += rows;
	    rows = 0;
	} else {
	    if (combline.para < parags_count() - 1) {
		// We have way to go. Move to the next paragraph,
		// decrease rows, and let the loop handle the rest.
		combline.para++;
		combline.inner_line = 0;
		rows -= remaining_line_breaks + 1;
	    } else {
		// We are at the last paragraph. Move to the last line.
		combline.inner_line = paragraphs[combline.para]->breaks_count() - 1;
		rows = 0;
	    }	    
	}

	if (rows != 0) // way to go?
	    add_rows_to_line(combline, rows);
    }

    if (rows < 0) {
	if (-rows <= combline.inner_line) {
	    // Our final position is inside the paragraph.
	    // We zero rows to finish the loop.
	    combline.inner_line -= -rows;
	    rows = 0;
	} else {
	    if (combline.para > 0) {
		// We have way to go. Move to the previous paragraph, increase
		// rows (towards zero), and let the loop handle the rest.
		combline.para--;
		rows += combline.inner_line + 1;
		combline.inner_line = paragraphs[combline.para]->breaks_count() - 1;
	    } else {
		// We are at the first paragraph. Move to the first line.
		combline.inner_line = 0;
		rows = 0;
	    }	    
	}

	if (rows != 0) // way to go?
	    add_rows_to_line(combline, rows);
    }
}

// lines_diff() - calculates the difference, in screen lines, between
// two CombinedLines.

int EditBox::lines_diff(CombinedLine L1, CombinedLine L2)
{
    if (L2 < L1)
	L2.swap(L1);

    if (L1.para == L2.para)
	return L2.inner_line - L1.inner_line;

    int rows = paragraphs[L1.para]->breaks_count() - L1.inner_line;
    L1.para++;
    while (L1.para < L2.para) {
	rows += paragraphs[L1.para]->breaks_count();
	L1.para++;
    }
    rows += L2.inner_line;

    return rows;
}

// scroll_to_cursor_line() - makes sure the cursor line (=current line) is
// visible on screen. If not, it scrolls. It is called at the end of various
// movement commands.

void EditBox::scroll_to_cursor_line()
{   
    CombinedLine curr(cursor.para, calc_inner_line());

    if (curr < top_line) {
	// if the current line is smaller than top_line, it means it's outside
	// the window view and we have to scroll up to make it visible.

	// calculate the new top line
	CombinedLine old_top_line = top_line;
	top_line = curr;
	add_rows_to_line(top_line, -(get_effective_scroll_step() - 1));

	// Performance: we can either repaint all the window (rgnAll),
	// or use curses' wscrl() and paint only the lines that aren't already
	// on screen.
	if (top_line.is_near(old_top_line)) {
	    int diff = lines_diff(top_line, old_top_line);
	    scrollok(wnd, TRUE);
	    wscrl(wnd, -diff);
	    scrollok(wnd, FALSE);
	    request_update(top_line.para, old_top_line.para - 1);
	} else {
	    request_update(rgnAll);
	}
	
	return;
    }

    // We don't need to scroll up. Now we check whether we need to scroll down.

    // calculate the bottom line
    CombinedLine bottom_line = top_line;
    add_rows_to_line(bottom_line, window_height() - 1);
	
    if (curr > bottom_line) {
	// if the current line is past bottom_line, we scroll down to make
	// it visible.
	
	// calculate the new bottom line
	bottom_line = curr;
	add_rows_to_line(bottom_line, get_effective_scroll_step() - 1);
	
	// and the new top line
	CombinedLine old_top_line = top_line;
	top_line = bottom_line;
	add_rows_to_line(top_line, -(window_height() - 1));

	// Performance: either repaint everything or just the lines that
	// aren't already on screen.
	if (top_line.is_near(old_top_line)) {
	    int diff = lines_diff(top_line, old_top_line);
	    CombinedLine from = bottom_line;
	    add_rows_to_line(from, -(diff - 1));
	    scrollok(wnd, TRUE);
	    wscrl(wnd, diff);
	    scrollok(wnd, FALSE);
	    request_update(from.para, bottom_line.para);
	} else {
	    request_update(rgnAll);
	}
    }
}

// move_forward_page() - interactive command called in response to the
// "Page Down" key. It mimics emacs.

INTERACTIVE void EditBox::move_forward_page()
{
    // First, check whether the end of the buffer is already on screen (by
    // calculating the bottom line). If so, move to the last line and quit.
    
    CombinedLine bottom_line = top_line;
    add_rows_to_line(bottom_line, window_height() - 1);
   
    if (lines_diff(top_line, bottom_line) < window_height() - 1) {
	move_relative_line(window_height());
	return;
    }
	
    // No, the end of the buffer is not on screen. Advance top_line by
    // window_height() lines (but keep 2 lines of context).
    add_rows_to_line(top_line, window_height() - 2);

    // if the cursor is now outside the view ...
    if (CombinedLine(cursor.para, calc_inner_line()) < top_line) {
	// ... then move it to the top line.
	cursor.para = top_line.para;
	cursor.pos  = (top_line.inner_line > 0)
			? curr_para()->line_breaks[top_line.inner_line - 1]
			: 0;
	invalidate_optimal_vis_column();
    }

    post_vertical_movement();
    request_update(rgnAll);
}

// move_backward_page() - interactive command called in response to the
// "Page Up" key. It mimics emacs.

INTERACTIVE void EditBox::move_backward_page()
{
    // First, check whether the first line of the buffer if already shown.  If
    // so, move to that first line and quit.

    if (top_line.para == 0 && top_line.inner_line == 0) {
	move_relative_line(-window_height());
	return;
    }
   
    // No, we can go back. Substract window_height() lines (minus 2 lines of
    // context) from top_line.
    add_rows_to_line(top_line, -(window_height() - 2));

    // Calculate the new bottom line
    CombinedLine bottom_line = top_line;
    add_rows_to_line(bottom_line, window_height() - 1);
  
    // if the cursor is now past the bottom line ...
    if (CombinedLine(cursor.para, calc_inner_line()) > bottom_line) {
	// ... then move it to the bottom line.
	cursor.para = bottom_line.para;
	cursor.pos  = (bottom_line.inner_line > 0)
			? curr_para()->line_breaks[bottom_line.inner_line - 1]
			: 0;
	invalidate_optimal_vis_column();
    }

    post_vertical_movement();
    request_update(rgnAll);
}

// center_line() - interactive command used to center the current line.
// (like emacs's C-l)

INTERACTIVE void EditBox::center_line()
{
    // Calculate new top line
    CombinedLine new_top_line(cursor.para, calc_inner_line());
    add_rows_to_line(new_top_line, -(window_height() / 2));
    top_line = new_top_line;
    request_update(rgnAll);
}
    
// }}}

// Movement commands {{{

// post_horizontal_movement() -- should be called at the end of horizontal
// movement commands. It makes sure the cursor is on screen and notify our
// listeners.

void EditBox::post_horizontal_movement()
{
    scroll_to_cursor_line();
    request_update(rgnCursor);
    invalidate_optimal_vis_column();
    NOTIFY_CHANGE(position);
}

// post_vertical_movement() -- should be called at the end of vertical
// movement commands.

void EditBox::post_vertical_movement()
{
    scroll_to_cursor_line();
    request_update(rgnCursor);
    NOTIFY_CHANGE(position);
}

// move_forward_char() - interactive command to move to the next logical
// character.

INTERACTIVE void EditBox::move_forward_char()
{
    if (cursor.pos < curr_para()->str.len()) {
        cursor.pos++;
    } else if (cursor.para < parags_count() - 1) {
	cursor.para++;
	cursor.pos = 0;
    }
    post_horizontal_movement();
}

// move_backward_char() - interactive command to move the the previous logical
// character.

INTERACTIVE void EditBox::move_backward_char()
{
    if (cursor.pos > 0) {
        cursor.pos--;
    } else if (cursor.para > 0) {
	cursor.para--;
	cursor.pos = curr_para()->str.len();
    }
    post_horizontal_movement();
}

// key_left() - responds to the "left arrow" key.

INTERACTIVE void EditBox::key_left()
{
    if (curr_para()->is_rtl()) {
	if (visual_cursor_movement)
	    move_forward_visual_char();
	else
	    move_forward_char();
    } else {
	if (visual_cursor_movement)
	    move_backward_visual_char();
	else
	    move_backward_char();
    }
}

// key_left() - responds to the "right arrow" key.

INTERACTIVE void EditBox::key_right()
{
    if (curr_para()->is_rtl()) {
	if (visual_cursor_movement)
	    move_backward_visual_char();
	else
	    move_backward_char();
    } else {
	if (visual_cursor_movement)
	    move_forward_visual_char();
	else
	    move_forward_char();
    }
}

bool EditBox::is_at_end_of_buffer() {
    return cursor.para == parags_count() - 1
	    && cursor.pos == curr_para()->str.len();
}

bool EditBox::is_at_beginning_of_buffer() {
    return cursor.para == 0 && cursor.pos == 0;
}

unichar EditBox::get_current_char()
{
    if (cursor.pos < curr_para()->str.len())
	return curr_para()->str[cursor.pos];
    else
	return get_curr_eop_char();
}

// move_forward_word() -- interactive command to move to the end of the
// current or next word.

INTERACTIVE void EditBox::move_forward_word()
{
    while (!BiDi::is_wordch(get_current_char()) && !is_at_end_of_buffer())
	move_forward_char();
    while (BiDi::is_wordch(get_current_char()) && !is_at_end_of_buffer())
	move_forward_char();
}

bool EditBox::is_at_beginning_of_word()
{
    if (cursor.pos == curr_para()->str.len())
	return false;
    if (cursor.pos == 0)
	return BiDi::is_wordch(curr_para()->str[0]);
    return (BiDi::is_wordch(curr_para()->str[cursor.pos])
		&& !BiDi::is_wordch(curr_para()->str[cursor.pos-1]));
}

// move_backward_word() -- interactive command to move to the start of the
// current or previous word.

INTERACTIVE void EditBox::move_backward_word()
{
    if (is_at_beginning_of_word())
	move_backward_char();
    while (!is_at_beginning_of_word() && !is_at_beginning_of_buffer())
	move_backward_char();
}

// move_beginning_of_buffer() -- interactive command to move to the beginning
// of the buffer

INTERACTIVE void EditBox::move_beginning_of_buffer()
{
    cursor.zero();
    post_vertical_movement();
}

// move_end_of_buffer() -- interactive command to move to the end of the
// buffer

INTERACTIVE void EditBox::move_end_of_buffer()
{
    cursor.para = parags_count() - 1;
    cursor.pos  = 0;
    post_vertical_movement();
}

// move_beginning_of_line() - interactive command to move to the beginning of
// the current screen line.

INTERACTIVE void EditBox::move_beginning_of_line()
{
    idx_t prev_line_break = 0;
    for (int line_num = 0; line_num < curr_para()->breaks_count(); line_num++) {
	idx_t line_break = curr_para()->line_breaks[line_num];
	if (cursor.pos < line_break
	    || (cursor.pos == line_break
		&& cursor.pos == curr_para()->str.len())) {
	    cursor.pos = prev_line_break;
	    break;
	}
	prev_line_break = line_break;
    }
    post_horizontal_movement();
}

// move_end_of_line() - interactive command to move to the end of
// the current screen line.

INTERACTIVE void EditBox::move_end_of_line()
{
    for (int line_num = 0; line_num < curr_para()->breaks_count(); line_num++) {
	idx_t line_break = curr_para()->line_breaks[line_num];
	if (cursor.pos < line_break) {
	    cursor.pos = line_break;
	    if (line_num != curr_para()->breaks_count() - 1)
		cursor.pos--;
	    break;
	}
    }
    post_horizontal_movement();
}

INTERACTIVE void EditBox::move_last_modification()
{
    if (last_modification.para || last_modification.pos)
	set_cursor_position(last_modification);
}

// set_cursor_position(Point) - allow other objects to position the cursor
// at a specific paragraph and column.

void EditBox::set_cursor_position(const Point &point)
{
    cursor.para = MIN(MAX(point.para, 0), parags_count() - 1);
    cursor.pos  = MIN(MAX(point.pos, 0),  curr_para()->str.len());
    post_horizontal_movement();
}

// move_absolute_line() -- jumps to a specific paragraph (used by the user when
// he knows where he wants to jump to, i.e. when reading compiler error messages).
    
void EditBox::move_absolute_line(int line)
{
    set_cursor_position(Point(line, 0));
}

// move_relative_line(diff) - moves the cursor diff lines up or down.

void EditBox::move_relative_line(int diff)
{
    if (!valid_optimal_vis_column())
	optimal_vis_column = calc_vis_column();

    CombinedLine curr(cursor.para, calc_inner_line());
    add_rows_to_line(curr, diff);
    cursor.para = curr.para;
    // move the cursor to the beginning of inner_line
    cursor.pos  = (curr.inner_line > 0)
			? curr_para()->line_breaks[curr.inner_line - 1]
			: 0;
    // reposition the cursor at the right column
    move_to_vis_column(optimal_vis_column);
    post_vertical_movement();
}

// move_next_line() - interactive command to move the cursor to the next
// screen line (e.g. in response to "key down")

INTERACTIVE void EditBox::move_next_line()
{
    move_relative_line(1);
}

// move_previous_line() - interactive command to move the cursor to the
// previous screen line (e.g. in response to "key up")

INTERACTIVE void EditBox::move_previous_line()
{
    move_relative_line(-1);
}

// search_forward() - a simple method for searching. It's not adequate for
// most uses: it can't ignore non-spacing marks and there's no corresponding
// method for searching backwards. I'll have to work on that.
//
// returns true if the string was found.

bool EditBox::search_forward(unistring str)
{
    str = str.toupper_ascii();
    const unichar *first, *last;
    for (int i = cursor.para; i < parags_count(); i++) {
	const unistring para = paragraphs[i]->str.toupper_ascii();
	first = para.begin();
	last = para.end();
	if (i == cursor.para) {
	    first += cursor.pos;
	    // skip the character under the cursor
	    if (first != last)
		first++;
	}
	const unichar *pos = std::search(first, last, str.begin(), str.end());
	if (pos != last) {
	    cursor.pos  = pos - para.begin();
	    cursor.para = i;
	    post_horizontal_movement();
	    return true;
	}
    }
    return false;
}

// move_first_char(ch) - Moves to the first ch in the buffer. When saving the
// buffer to a file the charset conversion may fail. This method is used to
// position the cursor on the offending character.

void EditBox::move_first_char(unichar ch)
{
    for (int i = 0; i < parags_count(); i++) {
	Paragraph &para = *paragraphs[i];
	idx_t len = para.str.len();
	for (int pos = 0; pos <= len; pos++)
	    if (
		 (pos  < len && para.str[pos] == ch) ||
		 (pos == len && para.eop != eopNone
			     && get_eop_char(para.eop) == ch)
							      )
	    {
		cursor.para = i;
		cursor.pos  = pos;
		post_horizontal_movement();
		return;
	    }
    }
}

// }}}

// Visual movement commands {{{

void EditBox::set_visual_cursor_movement(bool v)
{
    visual_cursor_movement = v;
    NOTIFY_CHANGE(selection); // I don't have a special notification for this event.
}

INTERACTIVE void EditBox::toggle_visual_cursor_movement()
{
    set_visual_cursor_movement(!get_visual_cursor_movement());
}

bool EditBox::is_at_end_of_screen_line()
{
    idx_t end_pos = curr_para()->str.len();
    for (int line_num = 0; line_num < curr_para()->breaks_count(); line_num++) {
	idx_t line_break = curr_para()->line_breaks[line_num];
	if (cursor.pos < line_break) {
	    end_pos = line_break;
	    if (line_num != curr_para()->breaks_count() - 1)
		end_pos--;
	    break;
	}
    }
    return cursor.pos == end_pos;
}

bool EditBox::is_at_first_screen_line()
{
    return cursor.para == 0 && calc_inner_line() == 0;
}

bool EditBox::is_at_last_screen_line()
{
    return (cursor.para == parags_count()-1)
		&& (calc_inner_line() == curr_para()->breaks_count()-1);
}

// move_forward_visual_char() - moves a char forward, visually.

INTERACTIVE void EditBox::move_forward_visual_char()
{
    int vis_column = calc_vis_column();
    if (is_at_end_of_screen_line()) {
	// move to visual start of next screen line
	if (!is_at_last_screen_line()) {
	    move_relative_line(1);
	    move_to_vis_column(0);
	}
    } else {
	move_to_vis_column(vis_column+1);
	if (calc_vis_column() == vis_column) {
	    // We're stuck on a tab or on a wide character.
	    // Ugly hack, sorry :-(
	    move_forward_char();
	}
    }
    post_horizontal_movement();
}

// move_backward_visual_char() - moves a char backward, visually.

INTERACTIVE void EditBox::move_backward_visual_char()
{
    int vis_column = calc_vis_column();
    if (vis_column == 0  // we need to move up a line,
	    && !is_at_first_screen_line())  // but only if we're not
					    // already at start of buffer
    {
	move_relative_line(-1);
	move_end_of_line();
    } else {
	move_to_vis_column(vis_column-1);
    }
    post_horizontal_movement();
}

INTERACTIVE void EditBox::move_beginning_of_visual_line()
{
    move_to_vis_column(0);
    post_horizontal_movement();
}

INTERACTIVE void EditBox::key_home()
{
    if (visual_cursor_movement)
	move_beginning_of_visual_line();
    else
	move_beginning_of_line();
}

// }}}

// Typing related: justification / indentation / alt_kbd / translation {{{

// insert_char() - inserts a character into the buffer. It assumes the
// character was typed by the user, so it also handles auto_justify.
// If you want to insert a character without this extra processing, use
// insert_text() instead.

void EditBox::insert_char(unichar ch)
{
    insert_text(&ch, 1);

    if (auto_justify && cursor.pos > justification_column) {
	// find previous whitespace
	idx_t wspos = cursor.pos - 1;
	while (wspos > 0 && !BiDi::is_space(curr_para()->str[wspos]))
		--wspos;
	if (wspos > 0) {
	    idx_t new_cursor_pos = cursor.pos - wspos - 1;
	    cursor.pos = wspos;
	    ch = get_curr_eop_char();
	    if (rfc2646_trailing_space) {
		// keep the space at the end of the line
		cursor.pos++;
		insert_text(&ch, 1);
		cursor.pos = new_cursor_pos;
	    } else {
		// overwrite the space with a new-line
		replace_text(&ch, 1, 1);
		cursor.pos = new_cursor_pos;
	    }
	    scroll_to_cursor_line();
	}
    }

    last_modification = cursor;
}

// key_enter() - called when the user hits Enter 

void EditBox::key_enter()
{
    if (auto_indent) {
	unistring indent;
	unistring &para = curr_para()->str;
	indent.push_back(get_curr_eop_char());
	for (idx_t i = 0; i < cursor.pos && BiDi::is_space(para[i]); i++)
	    indent.push_back(para[i]);
	insert_text(indent);
    } else {
	insert_char(get_curr_eop_char());
    }
}

// key_enter() - interactive command to insert maqaf into the buffer

INTERACTIVE void EditBox::insert_maqaf()
{
    insert_char(UNI_HEB_MAQAF);
}

// key_dash() - called when the user types '-'

void EditBox::key_dash()
{
    // if the previous [non-NSM] char is RTL and we're in smart_typing mode,
    // insert maqaf.
    if (smart_typing && cursor.pos > 0
	    && (BiDi::is_rtl(curr_para()->str[cursor.pos-1])
		|| (BiDi::is_nsm(curr_para()->str[cursor.pos-1])
		    && cursor.pos > 1
		    && BiDi::is_rtl(curr_para()->str[cursor.pos-2]))) )
	insert_maqaf();
    else
	insert_char('-');
}

void EditBox::set_translation_mode(bool value)
{
    if (!transtbl.empty()) {
	translation_mode = value;
	NOTIFY_CHANGE(translation_mode);
    } else {
	NOTIFY_ERROR(no_translation_table);
    }
}

void EditBox::set_alt_kbd(bool val)
{
    if (!altkbdtbl.empty()) {
	alt_kbd = val;
	NOTIFY_CHANGE(alt_kbd);
    }
}

INTERACTIVE void EditBox::toggle_alt_kbd()
{
    if (!altkbdtbl.empty()) {
	set_alt_kbd(!get_alt_kbd());
    } else {
	NOTIFY_ERROR(no_alt_kbd);
    }
}

// handle_event() - deals with literal keys. Special keys (control & alt
// combinations, function keys, arrows, etc) are dealt by the base class,
// Dispatcher. 
// 
// When "tranlation mode" is active, it translates the character according to
// the "transtbl" table. When "alt_kbd" is active -- according to the
// "altkbdtbl" table.

bool EditBox::handle_event(const Event &evt)
{
    current_command_type = cmdtpUnknown;
    if (evt.is_literal()) {
	unichar ch = evt.ch;
	if (in_translation_mode()) {
	    transtbl.translate_char(ch);
	    set_translation_mode(false);
	} else if (alt_kbd) {
	    altkbdtbl.translate_char(ch);
	}
	switch (ch) {
	case 13:  key_enter(); break; 
	case '-': key_dash();  break;
	default:
	    insert_char(ch);
	}
	prev_command_type = current_command_type;
	return true;
    } else {
	bool ret = Dispatcher::handle_event(evt);
	prev_command_type = current_command_type;
	return ret;
    }
}

static bool is_empty_or_starts_with_space(const unistring &str) {
    return (str.empty() || BiDi::is_space(str[0]));
}

static bool is_blank(const unistring &str) {
    for (idx_t i = 0; i < str.len(); i++)
	if (!BiDi::is_space(str[i]))
	    return false;
    return true;
}

// justify() - interactive command to justify the current "paragraph". Here
// "paragraph" means a bunch of Paragraphs (what a user thinks of as "lines")
// separated by blank or indented Paragraphs.

INTERACTIVE void EditBox::justify()
{
    // Step 1.
    // Move to a non-blank line 

    while (cursor.para < parags_count() - 1
		&& is_blank(curr_para()->str))
	move_next_line();

    // Step 2.
    // Find the min and max lines that constitute this paragraph.

    int min_para, max_para;
    min_para = max_para = cursor.para;
    // paragraphs are separated by blank lines or indentations.
    while (min_para > 0
	    && !is_empty_or_starts_with_space(paragraphs[min_para - 1]->str))
	--min_para;
    while (max_para < parags_count() - 1
	    && !is_empty_or_starts_with_space(paragraphs[max_para + 1]->str))
	++max_para;
    // if we're an indented paragraph, include the indented line.
    if (min_para > 0
	    && !is_empty_or_starts_with_space(paragraphs[min_para]->str)
	    && !is_blank(paragraphs[min_para - 1]->str))
	--min_para;
  
    // Step 3.
    // Collect all the lines into 'text'

    unistring text;
    for (int i = min_para; i <= max_para; i++) {
	if (!text.empty() && !BiDi::is_space(text.back()))
	    text.push_back(' ');
	text.append(paragraphs[i]->str);
    }
  
    // Step 4.
    // Justify 'text' into 'justified'

    unistring justified;
    while (!text.empty()) {
	if (!justified.empty()) {
	    if (rfc2646_trailing_space)
		justified.push_back(' ');
	    justified.push_back(get_curr_eop_char());
	}
	int wspos = justification_column;
	if (wspos >= text.len()) {
	    justified.append(text);
	    text.clear();
	} else {
	    while (wspos > 0 && !BiDi::is_space(text[wspos]))
		    --wspos;
	    if (wspos > 0) {
		justified.append(text, wspos);
		text.erase_head(wspos + 1);
	    } else {
		justified.append(text, justification_column);
		text.erase_head(justification_column);
	    }
	}
    }

    // Step 5.
    // Delete the unjustified lines from the buffer and insert
    // the 'justified' string instead.

    cursor.pos  = 0;
    cursor.para = min_para;
    int delete_len = calc_distance(cursor,
			Point(max_para, paragraphs[max_para]->str.len()));
    replace_text(justified, delete_len);
    
    // Move past this paragraph, so that the next "justify" command
    // justifies the next paragraph and so on.
    move_next_line();
    move_beginning_of_line();
}

// }}}

// Paragraph Base Direction {{{

void EditBox::set_dir_algo(diralgo_t value)
{
    dir_algo = value;
    for (int i = 0; i < parags_count(); i++)
	paragraphs[i]->determine_base_dir(dir_algo);
    if (dir_algo == algoContextStrong || dir_algo == algoContextRTL)
	calc_contextual_dirs(0, parags_count() - 1, false);
    request_update(rgnAll);
    NOTIFY_CHANGE(dir_algo);
}

// toggle_dir_algo() - interactive command to toggle the directionality
// algorithm used.

INTERACTIVE void EditBox::toggle_dir_algo() {
    switch (dir_algo) {
    case algoUnicode:  set_dir_algo(algoContextStrong); break;
    case algoContextStrong: set_dir_algo(algoContextRTL);  break;
    case algoContextRTL:    set_dir_algo(algoForceLTR);  break;
    case algoForceLTR: set_dir_algo(algoForceRTL); break;
    case algoForceRTL: set_dir_algo(algoUnicode);  break;
    }
}
    
// calc_contextual_dirs() - this method implements the "contextual" algorithm.
// Other algorithms are implemented in BiDi::determine_base_dir(), but the
// BiDi class doesn't know about the context of the paragraph, so we have to
// do most of the work in EditBox.
//
// The contextual algorithm has the following effect:
// 
// 1. If there's an RTL character in the paragraph, set the base
//    direction to RTL;
// 2. Else: if there's an LTR character in the paragraph, set the base
//    direction to LTR;
// 3. Else: the paragraph is neutral; set the base direction to the
//    base direction of the previous non-neutral paragraph. If all
//    previous paragraphs are neuteral, set it to the base direction
//    of the next non-neutral paragraph. If all paragraphs are
//    neutral, set it to LTR.
//    
// #1 and #2 are implemented in BiDi::determine_base_dir(). #3 is implemented
// here.

void EditBox::calc_contextual_dirs(int min_para, int max_para,
				   bool update_display)
{
    // find the previous strong paragraph
    while (min_para > 0) {
	min_para--;
	if (paragraphs[min_para]->individual_base_dir != dirN)
	    break;
    }
    // find the next strong paragraph
    while (max_para < parags_count() - 1) {
	max_para++;
	if (paragraphs[max_para]->individual_base_dir != dirN)
	    break;
    }

    // loop from min_para to max_para and whenever you find a neutral
    // paragraph, set its base direction to the last strong direction you
    // encountered (prev_strong).
    direction_t prev_strong = dirN;
    for (int i = min_para; i <= max_para; i++) {
	Paragraph &p = *paragraphs[i];
	if (p.individual_base_dir == dirN) {
	    set_contextual_dir(p, prev_strong, update_display);
	} else {
	    set_contextual_dir(p, p.individual_base_dir, update_display);
	    prev_strong = p.individual_base_dir;
	}
    }

    // Do the same but now loop backwards: from max_para to min_para.
    prev_strong = dirN;
    for (int i = max_para; i >= min_para; i--) {
	Paragraph &p = *paragraphs[i];
	if (p.contextual_base_dir == dirN) {
	    set_contextual_dir(p, prev_strong, update_display);
	} else {
	    prev_strong = p.contextual_base_dir;
	}
    }

    // If there're any neutral paragraphs left, set their base direction to
    // LTR
    for (int i = min_para; i <= max_para; i++) {
	Paragraph &p = *paragraphs[i];
	if (p.contextual_base_dir == dirN)
	    set_contextual_dir(p, dirLTR, update_display);
    }
}

inline void EditBox::set_contextual_dir(Paragraph &p, direction_t dir,
					bool update_display)
{
    if (!update_display)
	p.contextual_base_dir = dir;
    else {
	if (p.contextual_base_dir != dir) {
	    // if only the current paragraph has changed its direction,
	    // we don't need to repaint all the window.
	    p.contextual_base_dir = dir;
	    if (&p == curr_para())
		request_update(rgnCurrent);
	    else
		request_update(rgnAll);
	}
    }
}

// }}}

// Modification {{{

// post_modification() - should be called at the end of any primitive command
// that changes the buffer. It invalidates the selection, the optimal visual
// column and notifies our listeners.

void EditBox::post_modification()
{
    invalidate_optimal_vis_column();
    last_modification = cursor;
    cache.invalidate();
    if (is_primary_mark_set())
	unset_primary_mark();
    if (!is_modified())
	set_modified(true);
    NOTIFY_CHANGE(position);
}

// post_para_modification() - should be called for every paragraph that has
// been modified. It recalculates its base direcion and rewraps it.

void EditBox::post_para_modification(Paragraph &p)
{
    p.determine_base_dir(dir_algo);
    wrap_para(p);
}

// delete_forward_char() - interactive command to delete the character
// we're on. Usually bound to the DELETE key.

INTERACTIVE void EditBox::delete_forward_char()
{
    delete_text(1);
}

// delete_backward_char() - interactive command to delete the previous
// character. Usually bound to the BACKSPACE key.

INTERACTIVE void EditBox::delete_backward_char()
{
    if (!is_at_beginning_of_buffer()) {
	move_backward_char();
	delete_forward_char();
    }
}

// cut_end_of_paragraph() - interactive command to cut till
// end of paragraph. If the cursor is already at the end, it deletes
// the EOP (ala emacs).

INTERACTIVE void EditBox::cut_end_of_paragraph()
{
    int count;
    if (cursor.pos == curr_para()->str.len())
	count = 1;
    else
	count = curr_para()->str.len() - cursor.pos;

    Point orig_cursor = cursor;
    copy_text(count, (prev_command_type == cmdtpKill));
    cursor = orig_cursor;
    delete_text(count);

    current_command_type = cmdtpKill;
}

// delete_paragraph() - interactive command to delete the current
// paragraph.

INTERACTIVE void EditBox::delete_paragraph()
{
    cursor.pos = 0;
    delete_text(curr_para()->str.len() + 1);
}

// delete_forward_word() - interactive command to delete till the
// end of the current or next word.

INTERACTIVE void EditBox::delete_forward_word()
{
    Point start = cursor;
    move_forward_word();
    int len = calc_distance(cursor, start);
    cursor = start;
    delete_text(len);
}

// delete_backward_word() - interactive command to delete till the
// start of the current or previous word.

INTERACTIVE void EditBox::delete_backward_word()
{
    Point end = cursor;
    move_backward_word();
    int len = calc_distance(cursor, end);
    delete_text(len);
}

// replace_text() - is a combination of delete_text() and insert_text(). we
// should use it instead of calling these two methods separately because it
// records these two operations on the undo stack as one atomic operation.

void EditBox::replace_text(const unichar *str, int len, int delete_len,
			   bool skip_undo)
{
    unistring deleted;
    Point point = cursor;
    delete_text(delete_len, true, &deleted);
    insert_text(str, len, true);

    if (!skip_undo) {
	UndoOp op;
	op.type = opReplace;
	op.point = point;
	op.deleted_text = deleted;
	op.inserted_text.append(str, len);
	undo_stack.record_op(op);
    }
}

// delete_text() - deletes len characters, starting at the cursor.
//
// @param skip_undo - don't record this operation on the undo stack.
// @param alt_deleted - copy the deleted text here.

void EditBox::delete_text(int len, bool skip_undo, unistring *alt_deleted)
{
    if (read_only) {
	NOTIFY_ERROR(read_only);
	return;
    }
    
    unistring _deleted, *deleted;
    deleted = alt_deleted ? alt_deleted : &_deleted;
    
    int parags_deleted = 0; // we keep count of how many parags we delete.
    
    while (len > 0) {
	if (cursor.pos == curr_para()->str.len()) {
	    if (cursor.para < parags_count() - 1) {
		// Delete the EOP. that is, append the next
		// paragraph to the current paragraph.
		deleted->push_back(get_curr_eop_char());
		Paragraph *next_para = paragraphs[cursor.para + 1];
		curr_para()->str.append(next_para->str);
		curr_para()->eop = next_para->eop;
		delete next_para;
		paragraphs.erase(paragraphs.begin() + cursor.para + 1);
	    }
	    len--;
	    parags_deleted++;
	} else {
	    // delete the [cursor, end-of-paragraph) segment
	    int to_delete = MIN(curr_para()->str.len() - cursor.pos, len);
	    unichar *cursor_ptr = curr_para()->str.begin() + cursor.pos;
	    deleted->append(cursor_ptr, to_delete);
	    curr_para()->str.erase(cursor_ptr, cursor_ptr + to_delete);
	    len -= to_delete;
	}
    }

    // optimization: we save some values and latter check whether they changed.
    int orig_num_lines = curr_para()->breaks_count();
    direction_t orig_individual_base_dir = curr_para()->individual_base_dir; 

    post_para_modification(*curr_para());

    // if no para was deleted, and the direction of the current para has not
    // changed, it cannot affect the surrounding parags, so we don't call
    // calc_contextual_dirs()
    if (parags_deleted ||
	    curr_para()->individual_base_dir != orig_individual_base_dir)
	calc_contextual_dirs(cursor.para, cursor.para, true);

    // if no para was deleted, if no screen lines were deleted, we can
    // update only rgnCurrent instead of the whole window.
    if (!parags_deleted && curr_para()->breaks_count() == orig_num_lines)
    	request_update(rgnCurrent);
    else
	request_update(rgnAll);
   
    scroll_to_cursor_line();

    if (!skip_undo) {
	UndoOp op;
	op.type = opDelete;
	op.point = cursor;
	op.deleted_text = *deleted;
	undo_stack.record_op(op);
    }

    post_modification();
}

// insert_text() - inserts text into the buffer. advances the cursor.
//
// @param skip_undo - don't record this operation on the undo stack.

void EditBox::insert_text(const unichar *str, int len, bool skip_undo)
{
    if (read_only && !is_in_data_transfer()) {
	if (len)
	    NOTIFY_ERROR_ARG(read_only, str[0]);
	return;
    }
    
    if (!skip_undo) {
	UndoOp op;
	op.type = opInsert;
	op.point = cursor;
	op.inserted_text.append(str, len);
	undo_stack.record_op(op);
    }
    
    int min_changed_para, max_changed_para; // which parags have changed?
    min_changed_para = cursor.para;

    while (len > 0) {
	if (is_eop(str[0])) {
	    // inserting EOP is like pressing Enter: split the current
	    // paragraph into two.
	    Paragraph *p = new Paragraph();
	    p->str.insert(0, &curr_para()->str[cursor.pos],
			      curr_para()->str.end());
	    p->eop = curr_para()->eop;
	    curr_para()->str.erase(&curr_para()->str[cursor.pos],
				    curr_para()->str.end());
	    curr_para()->eop = get_eop_type(str[0]);
	    paragraphs.insert(paragraphs.begin() + cursor.para + 1, p);
	    cursor.para++;
	    cursor.pos = 0;
	    len--;
	    str++;
	} else {
	    int line_len = 0;
	    while (line_len < len && !is_eop(str[line_len]))
		line_len++;
	    curr_para()->str.insert(curr_para()->str.begin() + cursor.pos,
				    str, str + line_len);
	    cursor.pos += line_len;
	    len -= line_len;
	    str += line_len;
	}
    }
    max_changed_para = cursor.para;

    // optimization: we save some values and latter check whether they changed.
    int orig_num_lines = curr_para()->breaks_count();
    direction_t orig_individual_base_dir = curr_para()->individual_base_dir; 

    // update state variables of the affected parags.
    for (int i = min_changed_para; i <= max_changed_para; i++)
	post_para_modification(*paragraphs[i]);

    // if only one para was modified, and its direction has not changed,
    // it doesn't affect the surrounding parags, so we don't call
    // calc_contextual_dirs()
    if (!(min_changed_para == max_changed_para &&
	    curr_para()->individual_base_dir == orig_individual_base_dir))
	calc_contextual_dirs(min_changed_para, max_changed_para, true);
   
    // if only one para was modified, and no screen lines were added, we can
    // update only rgnCurrent instead of the whole window.
    if (min_changed_para == max_changed_para
	    && curr_para()->breaks_count() == orig_num_lines)
	request_update(rgnCurrent);
    else
	request_update(rgnAll);

    scroll_to_cursor_line();

    post_modification();
}

// }}}

