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

#ifndef BDE_INPUTLINE_H
#define BDE_INPUTLINE_H

#include <vector>

#include "editbox.h"


class InputLine : public EditBox {

public:

    typedef std::vector<unistring> StringArray;

    // What kind of filenames, if any, to complete?
    enum CompleteType {
	    cmpltOff,		// turn off filename completion.
	    cmpltAll,
	    cmpltDirectories	// complete only directory names.
				// this is useful for, e.g., a "Change Dir:"
				// input line.
	};

private:

    unistring	vis_label;
    direction_t	label_dir;
    int		label_width;

    // Filename completion variables:

    CompleteType complete_type;
    StringArray  files_list;
    u8string     files_directory;   // the directory we're listing

    int event_num;
    int last_tab_event_num;

    int insertion_pos;
    int prefix_len;
    int slice_begin;
    int slice_end;
    int curr_choice;

    /*
	A little example can help in explaining what each completion
	variable means.

	Let's suppose our input-line looks like:

	~/documents/no_ -cp862
		      ^-----------cursor

	We press TAB and a "TAB session" starts:

	0. "no" is the partial filename component and "~/documents/" is the
	   directory name component.

	1. files_list is populated with all filenames in "~/documents":

	    0. archive.txt
	    1. notes.txt
	    2. notes2.txt
	    3. november.txt
	    4. questions.txt
    
	2. only entries 1 to 3 are relevant in our TAB session (because
	   they start in "no"), so slice_begin is set to 1 and slice_end
	   to 3.
	
	3. insertion_pos is set to the cursor position.

	4. prefix_len is set to the length of the partial filename
	   component: 2. (that is, strlen("no")).

	5. While moving forward and backward in files_list (as a result
	   of pressing TAB and M-TAB), curr_choice is updated to point
	   to the current entry.
    */

    // History variables:

    StringArray *history;
    int history_idx;

protected:

    // Filename completion

    void init_completion();
    void get_directory_files(u8string directory, const char *prefix = NULL);
    void complete(bool forward);

    // History

    void init_history(int history_set);
    void update_history();

    ////

    void trim();

protected:
    virtual void do_syntax_highlight(const unistring &str, 
	    AttributeArray &attributes, int para_num);

public:

    HAS_ACTIONS_MAP(InputLine, EditBox);
    HAS_BINDINGS_MAP(InputLine, EditBox);

    InputLine(const char *aLabel, const unistring &default_text,
	      int history_set = 0, CompleteType complete = cmpltOff);
    void set_label(const char *aLabel);
    void set_text(const unistring &s);
    unistring get_text() { return curr_para()->str; }

    INTERACTIVE void end_modal();
    INTERACTIVE void next_completion();
    INTERACTIVE void previous_completion();
    INTERACTIVE void next_history();
    INTERACTIVE void previous_history();

    virtual void redraw_paragraph(Paragraph &p,
			    int window_start_line, bool only_cursor, int);
    
    virtual bool handle_event(const Event &evt);
};

#endif

