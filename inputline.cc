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

#if HAVE_DIRENT_H
# include <dirent.h> // for filename completion
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <algorithm>
#include <map>

#include "inputline.h"
#include "geresh_io.h" // expand_tilde
#include "themes.h"
#include "dbg.h"

// A history is stored as a StringArray. There are usually several histories
// in one application (for example, the history for an "Open file:" stores
// file names, and the history for a "Find:" stores search strings). The
// histories are stored in a history map, and an identifier, history_set, that
// is passed to InputLine's constructor, is used in selecting the appropriate
// history.

static std::map<int, InputLine::StringArray> histories;

// init_history() - initializes the "history" pointer and pushes an initial
// value into it. when history_set is zero, history is disabled for this
// InputLine object.

void InputLine::init_history(int history_set)
{
    if (history_set == 0) {
	history = NULL;
    } else {
	history = &histories[history_set];
	history_idx = 0;
    
	// Add the current text to history, provided it's not already
	// there; if the last entry is the empty string, overwrite
	// it instead of preserving it.
	if (history->empty()) {
	    history->insert(history->begin(), get_text());
	} else { 
	    if (history->front().empty())
		history->front() = get_text();
	    else if (get_text() != history->front())
		history->insert(history->begin(), get_text());
	}
    }
}

InputLine::InputLine(const char *aLabel, const unistring &default_text,
		     int history_set, CompleteType complete)
{
    set_label(aLabel);
    set_text(default_text);
    init_history(history_set);
    complete_type = complete;
    wrap_type = wrpOff;
    event_num = 0;
    last_tab_event_num = -2;
    set_modal();
}

void InputLine::set_label(const char *aLabel)
{
    unistring label;
    label.init_from_utf8(aLabel);
    label_dir = BiDi::determine_base_dir(label.begin(), label.size(), algoUnicode);
    BiDi::simple_log2vis(label, label_dir, vis_label);
    label_width = get_str_width(vis_label.begin(), vis_label.size());
    margin_before = 2 + label_width;
    margin_after  = 2;
}

void InputLine::set_text(const unistring &s)
{
    new_document();
    insert_text(s, true);
}

// redraw_paragraph() - override EditBox's method. it calls base and then
// draws the label.

void InputLine::redraw_paragraph(Paragraph &p,
			int window_start_line, bool only_cursor, int para_num)
{
    // If the label is RTL, it's printed on the right, else --
    // on the left. We use either margin_before or margin_after to
    // reserve the space for it.

    if ( (p.is_rtl() && label_dir == dirRTL)
	    || (!p.is_rtl() && label_dir == dirLTR) ) {
	margin_before = 2 + label_width;
	margin_after = 1;
    } else {
	margin_after = 2 + label_width;
	margin_before = 1;
    }
    
    // Curses bug: don't paint on the bottom-right cell, because
    // some Curses implementations get confused. 
    if (label_dir == dirLTR && !p.is_rtl()) {
	margin_after = 2;
    }

    EditBox::redraw_paragraph(p, window_start_line, only_cursor, para_num);

    if (!only_cursor) {
	// draw label
	if (label_dir == dirRTL) {
	    wmove(wnd, 0, window_width()
			    - (p.is_rtl() ? margin_before : margin_after)
			    + 1);
	    draw_unistr(vis_label.begin(), vis_label.size());
	} else {
	    wmove(wnd, 0, 1);
	    draw_unistr(vis_label.begin(), vis_label.size());
	}
	// reposition the cursor
	EditBox::redraw_paragraph(p, window_start_line, true, para_num);
    }
}

// get_directory_files() - populates files_list.
//
// @param directory - the directory to list.

void InputLine::get_directory_files(u8string directory, const char *prefix)
{
    expand_tilde(directory);
    files_list.clear();

    DIR *dir = opendir(directory.c_str());
    if (dir == NULL) {
	signal_error();
	return;
    }

    dirent *ent;
    u8string pathname = directory;
    int path_file_pos = pathname.size();

    while ((ent = readdir(dir))) {
	const char *d_name = ent->d_name;
	if (STREQ(d_name, ".") || STREQ(d_name, ".."))
	    continue;
	if (prefix && (strncmp(prefix, d_name, strlen(prefix)) != 0))
	    continue;
	pathname.erase(path_file_pos, pathname.size());
	pathname += d_name;

	struct stat file_info;
	stat(pathname.c_str(), &file_info);
	bool is_dir = S_ISDIR(file_info.st_mode);

	unistring filename;
	filename.init_from_filename(d_name);
	if (is_dir)
	    filename.push_back('/');

	if (complete_type == cmpltAll
		|| (complete_type == cmpltDirectories && is_dir))
	    files_list.push_back(filename);
    }
    closedir(dir);

    // it's important that this list be sorted.
    std::sort(files_list.begin(), files_list.end());
}

// init_completion() - initialize filename completion. it reads the directory
// and (partial) filename components under the cursor, calls
// get_directory_files() to list the files, and sets slice_begin and
// slice_end to the relevant range in files_list.

void InputLine::init_completion()
{
    trim();
    unistring &line = curr_para()->str;
    
    // get directory, filename components
    unistring filename;
    u8string  directory;
    
    int i = cursor.pos - 1;
    while (i >= 0 && line[i] != '/')
	--i;
    ++i; // move past '/'
    // get the directory component
    directory.init_from_unichars(line.begin(), i);
    if (directory.empty())
	directory = "./";
    // get the filename component
    filename.append(line.begin() + i, cursor.pos - i);

    if (0) {
	// if we haven't changed directory, no need to reread
	// directory contents.
	if (files_directory != directory) {
	    files_directory = directory;
	    get_directory_files(directory);
	}
    } else {
	// All in all, the following is faster, because it
	// doesn't stat(2) everything in the directory.
    	get_directory_files(directory, u8string(filename).c_str());
    }

    insertion_pos = cursor.pos;
    prefix_len = filename.len();
    
    slice_begin = slice_end = curr_choice = -1;
    for (unsigned i = 0; i < files_list.size(); i++) {
	unistring &entry = files_list[i];
	if (entry.len() >= prefix_len)
	    if (std::equal(filename.begin(), filename.end(), entry.begin())) {
		if (slice_begin == -1)
		    slice_begin = i;
		slice_end = i;
	    }
    }
}

// complete() - completes the filename under the cursor.
//
// @param forward - complete to the next filename if true; to the
// previous one if false.

void InputLine::complete(bool forward)
{
    // event_num and last_tab_event_num are used as a crude mechanism to
    // determine if a TAB session has ended, e.g. as a result of a key
    // other than TAB being pressed.
    //
    // On each event event_num is incremented (see handle_event()). if
    // last_tab_event_num is event_num shy 1, it means our TAB session
    // was not interrupted.

    bool restart = (event_num - 1 != last_tab_event_num);
    last_tab_event_num = event_num;

    if (restart) {
	// starts a TAB session.
	init_completion();
	// give warning when the length of the partial filename component
	// is 0. This means that we browse through _all_ the files in
	// the directory.
	if (prefix_len == 0)
	   signal_error();
    }

    if (slice_begin == -1) {
	// no filenames begin with the partial filename component.
	signal_error();
	return;
    }

    int prev_choice_len = 0;
    if (curr_choice != -1)
	prev_choice_len = files_list[curr_choice].size() - prefix_len;
	
    // wrap around the completion slice if we bumped into its end.
    if (curr_choice != -1)
    	curr_choice += forward ? 1 : -1;
    if (curr_choice > slice_end || curr_choice < slice_begin)
	curr_choice = -1;
    if (curr_choice == -1)
	curr_choice = forward ? slice_begin : slice_end;

    cursor.pos = insertion_pos;
    replace_text(files_list[curr_choice].begin() + prefix_len,
		    files_list[curr_choice].size() - prefix_len, prev_choice_len); 
    
    // a special case: if we've uniquely completed a direcory name, we want
    // the next TAB to start a new TAB session to complete the files within.
    if (slice_begin == slice_end
		&& *(files_list[curr_choice].end() - 1) == '/')
	last_tab_event_num = -2;
}

// previous_completion() - interactive command to move backward in the
// completion list. Usually bound to the M-TAB key.

INTERACTIVE void InputLine::previous_completion()
{
    if (complete_type != cmpltOff)
	complete(false);
}

// next_completion() - interactive command to move forward in the
// completion list. Usually bound to the TAB key.

INTERACTIVE void InputLine::next_completion()
{
    if (complete_type != cmpltOff)
	complete(true);
}

bool InputLine::handle_event(const Event &evt)
{
    // Emulate contemporary GUIs, where the input is
    // cleared on first letter typed.
    if (event_num == 0 && evt.is_literal() && evt.ch != 13) {
	// we don't use new_document() because we want to
	// be able to undo this operation.
	delete_paragraph();
    }
    
    if (event_num == 0)
	request_update(rgnAll); // make sure do_syntax_highlight is called.

    event_num++;

    if (!Dispatcher::handle_event(evt))
	return EditBox::handle_event(evt);
    return false;
}

// do_syntax_highlight() - Emulate comtemporary GUIs: when event_num is 0,
// select all the text to hint the user the next letter will erase everything.

void InputLine::do_syntax_highlight(const unistring &str, 
	AttributeArray &attributes, int para_num)
{
    if (event_num == 0) {
	attribute_t selected_attr = get_attr(EDIT_SELECTED_ATTR);
	for (idx_t i = 0; i < str.len(); i++)
	    attributes[i] = selected_attr;
    }
}

// trim() - removes the wspaces at the start and end of the text
// the user typed.

void InputLine::trim()
{
    unistring &line = curr_para()->str;
    // delete wspaces at start of line
    idx_t i = 0;
    while (i < line.len() && BiDi::is_space(line[i]))
	i++;
    if (i) {
	idx_t prev_pos = cursor.pos;
	cursor.pos = 0;
	delete_text(i);
	if (prev_pos >= i)
	    cursor.pos = prev_pos - i;
	request_update(rgnCurrent);
    }
    // delete wspaces at end of line
    i = line.len() - 1;
    while (i >= 0 && BiDi::is_space(line[i]))
	i--;
    i++;
    if (i < line.len()) {
	idx_t prev_pos = cursor.pos;
	cursor.pos = i;
	delete_text(line.len() - i);
	if (prev_pos < i)
	    cursor.pos = prev_pos;
	request_update(rgnCurrent);
    }
}

INTERACTIVE void InputLine::end_modal()
{
    trim();
    if (history) {
	// User presses Enter.
	// If the user has altered the default text, don't overwrite the
	// history entry but create a new entry intead.
	if (history_idx == 0 && get_text() != history->front()
		    && !history->front().empty())
	    history->insert(history->begin(), get_text());
	else
	    update_history();
    }
    EditBox::end_modal();
}

void InputLine::update_history()
{
    if (history)
	(*history)[history_idx] = get_text();
}

// previous_history() - interactive command to move to the previous history
// entry. Usually bound to the "Arrow Up" or C-p key.

INTERACTIVE void InputLine::previous_history()
{
    if (history && history_idx < (int)history->size() - 1) {
	update_history();
	history_idx++;
	set_text((*history)[history_idx]);
	event_num = 0;
    }
}

// next_history() - interactive command to move to the next history
// entry. Usually bound to the "Arrow Down" or C-n key.

INTERACTIVE void InputLine::next_history()
{
    if (history && history_idx > 0) {
	update_history();
	history_idx--;
	set_text((*history)[history_idx]);
	event_num = 0;
    }
}

