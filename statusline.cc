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

#include <string.h>

#include "statusline.h"
#include "editor.h"
#include "themes.h"
#include "dbg.h"

StatusLine::StatusLine(const Editor *aBde, EditBox *aEditbox)
{
    create_window();
    aEditbox->set_status_listener(this);
    editbox = aEditbox;
    bde = aBde;
    cursor_position_report = false;
    update_region = rgnAll;
}

void StatusLine::resize(int lines, int columns, int y, int x)
{
    Widget::resize(lines, columns, y, x);
    request_update(rgnAll);
}

void StatusLine::request_update(region rgn)
{
    update_region |= rgn;
}

void StatusLine::invalidate_view()
{
    request_update(rgnAll);
}

void StatusLine::toggle_cursor_position_report()
{
    cursor_position_report = !cursor_position_report;
    request_update(rgnAll);
}

void StatusLine::update()
{
    if (update_region & rgnFilename)
	request_update(rgnAll);

    if (update_region & rgnAll) {
	wbkgd(wnd, get_attr(STATUSLINE_ATTR));
	wmove(wnd, 0, 0);
	wclrtoeol(wnd);
    }
	
    if (update_region & (rgnAll | rgnIndicators)) {
	wmove(wnd, 0, 0);
	waddch(wnd, '[');
    
	waddch(wnd, editbox->is_modified() ? 'M' : '-');

	char wrap_ch = '-';
	switch (editbox->get_wrap_type()) {
	    case EditBox::wrpOff:	    wrap_ch = '$'; break;
	    case EditBox::wrpAnywhere:	    wrap_ch = '\\'; break;
	    case EditBox::wrpAtWhiteSpace:  wrap_ch = '-'; break; // silence the compiler
	}
	waddch(wnd, wrap_ch);
	
	char algo_ch = '-';
	switch (editbox->get_dir_algo()) {
	    case algoUnicode:	    algo_ch = '!'; break;
	    case algoContextStrong: algo_ch = '~'; break;
	    case algoContextRTL:    algo_ch = '-'; break;
	    case algoForceLTR:	    algo_ch = '>'; break;
	    case algoForceRTL:	    algo_ch = '<'; break;
	}
	waddch(wnd, algo_ch);
	
	waddch(wnd, editbox->has_selected_text()
			&& !bde->in_spelling() ? '@' : '-');

	waddch(wnd, editbox->is_read_only() ? 'R' : '-');
	
	waddch(wnd, bde->is_speller_loaded() ? 'S' : '-');

	waddch(wnd, editbox->in_translation_mode() ? '"' : '-');

	waddch(wnd, terminal::do_arabic_shaping ? 'a' : '-');

	waddch(wnd, editbox->get_alt_kbd() ? 'H' : '-');
	
	waddch(wnd, editbox->is_auto_justify() ? 'j' : '-');

	waddch(wnd, editbox->is_auto_indent() ? 'i' : '-');
	
	char maqaf_ch = '-';
	switch (editbox->get_maqaf_display()) {
	    case EditBox::mqfAsis:	     maqaf_ch = '-'; break;
	    case EditBox::mqfTransliterated: maqaf_ch = 'k'; break;
	    case EditBox::mqfHighlighted:    maqaf_ch = 'K'; break;
	}
	waddch(wnd, maqaf_ch);
	
	waddch(wnd, editbox->is_smart_typing() ? 'q' : '-');

	char rtlnsm_ch = '-';
	switch (editbox->get_rtl_nsm_display()) {
	    case EditBox::rtlnsmOff:		rtlnsm_ch = '-'; break;
	    case EditBox::rtlnsmTransliterated: rtlnsm_ch = 'n'; break;
	    case EditBox::rtlnsmAsis:		rtlnsm_ch = 'N'; break;
	}
	waddch(wnd, rtlnsm_ch);
	
	waddch(wnd, editbox->get_visual_cursor_movement() ? 'v' : '-');
	
	waddch(wnd, editbox->has_formatting_marks() ? 'F' : '-');

	waddch(wnd, ']');
    }

    if (update_region & (rgnAll | rgnFilename)) {
	unistring tmp;
	tmp.init_from_filename(bde->get_filename());
	u8string filename;
	filename.init_from_unichars(tmp);

	waddch(wnd, ' ');
	if (filename.empty()) {
	    draw_string(_("UNTITLED"));
	} else {
	    waddch(wnd, '"');
	    draw_string(filename.c_str());
	    waddch(wnd, '"');
	}
	if (bde->is_new())
	    draw_string(_(" [New File]"));
	wmove(wnd, 0, window_width() - strlen(bde->get_encoding()) - (2+5));
	wprintw(wnd, "[disk:%s]", bde->get_encoding());
    }

    if (cursor_position_report && (update_region & (rgnAll | rgnCursorPos))) {
	// we use 16 columns for the cursor position.
	int x = window_width() - strlen(bde->get_encoding()) - (2+5) - 16;
	wmove(wnd, 0, x);
	Point cursor;
	editbox->get_cursor_position(cursor);
        int total = editbox->get_number_of_paragraphs();
	wprintw(wnd, "%4d/%4d,%3d   ", total, cursor.para + 1, cursor.pos + 1);
    }
	    
    wnoutrefresh(wnd);
    update_region = rgnNone;
}

