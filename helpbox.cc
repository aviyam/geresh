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

#include "helpbox.h"
#include "editor.h"
#include "io.h"
#include "pathnames.h"
#include "themes.h"

std::vector<HelpBox::Position> HelpBox::positions_stack;

HelpBox::HelpBox(Editor *aApp, const EditBox &settings)
{
    app = aApp;
    statusmsg.set_text(_("F1 or ALT-X to exit help, ENTER to follow link, 'l' to move back, 't' to TOC"));
    statusmsg.highlight();
    
    set_undo_size_limit(0);
    set_read_only(true);

    scroll_step = settings.get_scroll_step();
    rtl_nsm_display = settings.get_rtl_nsm_display();
    maqaf_display = settings.get_maqaf_display();
    
    show_explicits = false;
    show_paragraph_endings = true;
    show_tabs = true;
}

INTERACTIVE void HelpBox::layout_windows()
{
    app->layout_windows();
    int cols, rows;
    getmaxyx(stdscr, rows, cols);
    resize(rows - 1, cols, 0, 0);
    statusmsg.resize(1, cols, rows - 1, 0);
}

bool HelpBox::load_user_manual()
{
    u8string dummy_encoding;
    bool dummy_new;
    if (!xload_file(this, get_cfg_filename(USER_MANUAL_FILE),
		NULL, "UTF-8", dummy_encoding, dummy_new, false)
	    && !xload_file(this, get_cfg_filename(SYSTEM_MANUAL_FILE),
		    NULL, "UTF-8", dummy_encoding, dummy_new, false))
    {
	app->show_file_io_error(_("Cannot open the manual file %s: %s"),
			get_cfg_filename(SYSTEM_MANUAL_FILE));
	return false;
    } else {
	scan_toc();
	pop_position();
	return true;
    }
}

void HelpBox::push_position()
{
    Position pos;
    pos.top_line = top_line;
    pos.cursor = cursor;
    positions_stack.push_back(pos);
}

INTERACTIVE void HelpBox::pop_position()
{
    if (positions_stack.size()) {
	Position pos = positions_stack.back();
	positions_stack.pop_back();
	top_line = pos.top_line;
	if ((int)paragraphs[top_line.para]->line_breaks.size()
		<= top_line.inner_line)
	    // window probably resized, so previous
	    // inner_line lost.
	    top_line.inner_line = 0;
	set_cursor_position(pos.cursor);
	request_update(rgnAll);
    }
}

INTERACTIVE void HelpBox::move_to_toc()
{
    push_position();
    top_line = CombinedLine(toc_first_line, 0);
    set_cursor_position(Point(toc_first_line, 0));
    request_update(rgnAll);
}

INTERACTIVE void HelpBox::refresh_and_center()
{
    center_line();
    invalidate_view();
    statusmsg.invalidate_view();
    clearok(curscr, TRUE);
}

INTERACTIVE void HelpBox::jump_to_topic(const char *topic)
{
    push_position();
    move_beginning_of_buffer();
    while (search_forward(unistring(u8string(topic))) && cursor.pos != 0)
	;
    top_line = CombinedLine(cursor.para, 0);
    request_update(rgnAll);
}

// HelpBox::exec() - reads events and hands them to the help window.

void HelpBox::exec()
{
    set_modal();
    while (is_modal()) {
	Event evt;
	statusmsg.update();
	update();
	doupdate();
	get_next_event(evt, wnd);
	handle_event(evt);
    }
    push_position();
}

// scan_toc() - reads the TOC (table of contents) into toc_items

void HelpBox::scan_toc()
{
    toc_items.clear();
    bool in_toc = false;
    for (int i = 0; i < parags_count(); i++) {
	const unistring &str = paragraphs[i]->str;
	if (!in_toc) {
	    if (str.len() && str[0] == '-') {
		in_toc = true;
		toc_first_line = i;
	    }
	}
	if (in_toc) {
	    if (str.len()) {
		// add this item to toc_items
		unistring item = str;
		while (item.len() &&
			(item[0] == ' ' || item[0] == '-'))
		    item.erase_head(1);
		toc_items.push_back(item);
	    } else {
		// finished processing TOC
		toc_last_line = i - 1;
		break;
	    }
	}
    }
}

// jump_to_topic() - reads the topic on which the cursor stands,
// then jumps to it. Usually called in reply to the ENTER key.

void HelpBox::jump_to_topic()
{
    const unistring &str = curr_para()->str;
    if (cursor.para >= toc_first_line && cursor.para <= toc_last_line) {
	unistring topic = str;
	while (topic.len()
		&& (topic[0] == ' ' || topic[0] == '-'))
	    topic.erase_head(1);
	jump_to_topic(u8string(topic).c_str());
    } else if (cursor.pos < str.len()) {
	// read the topic on which the cursor stands
	idx_t start = cursor.pos;
	while (start >= 0 && str[start] != '"')
	    start--;
	idx_t end = cursor.pos;
	while (end < str.len() && str[end] != '"')
	    end++;
	if (str[start] == '"' && str[end] == '"' && (start < end)) {
	    unistring topic = str.substr(start + 1, end - start - 1);
	    jump_to_topic(u8string(topic).c_str());
	}
    }
}

// do_syntax_highlight() - highlight all the topic names in the text.

void HelpBox::do_syntax_highlight(const unistring &str, 
	AttributeArray &attributes, int para_num)
{
    attribute_t links_attr = get_attr(EDIT_LINKS_ATTR);
    if (para_num >= toc_first_line && para_num <= toc_last_line) {
	// toc
	int i = 0;
	while (str[i] == ' ' || str[i] == '-')
	    i++;
	while (i < str.len())
	    attributes[i++] = links_attr;
    } else {
	// normal text: search for the topic names
	for (int i = 0; i < (int)toc_items.size(); i++) {
	    unistring topic = toc_items[i];
	    topic.insert(topic.begin(), '"');
	    topic.push_back('"');
	    idx_t pos = 0;
	    while ((pos = str.index(topic, pos + 1)) != -1) {
		for (idx_t k = pos + 1; k < pos + topic.len() - 1; k++)
		    attributes[k] = links_attr;
	    }
	}
    }
}

bool HelpBox::handle_event(const Event &evt)
{
    if (evt.is_literal()) {
	switch (evt.ch) {
	case 13:
	    jump_to_topic();
	    return true;
	case 'l':
	    pop_position();
	    return true;
	case 't':
	case 'd':
	    move_to_toc();
	    return true;
	case 'q':
	    end_modal();
	    return true;
	}
    }
    if (!Dispatcher::handle_event(evt))
	return EditBox::handle_event(evt);
    return false;
}

