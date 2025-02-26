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

#ifndef BDE_STATUSLINE_H
#define BDE_STATUSLINE_H

#include "editbox.h"

class Editor;

class StatusLine : public Widget, public EditBoxStatusListener {

    const EditBox *editbox;
    const Editor *bde;

    enum region { rgnNone = 0, rgnIndicators = 1, rgnFilename = 2,
		  rgnCursorPos = 4, rgnAll = 8 };
    int update_region;

    bool cursor_position_report; // report cursor position?

public:

    StatusLine(const Editor *aBde, EditBox *aEditbox);

    void toggle_cursor_position_report();
    bool is_cursor_position_report() const { 
	return cursor_position_report;
    }

    void request_update(region rgn);
    virtual void update();
    virtual void invalidate_view();
    virtual bool is_dirty() const { return update_region != rgnNone; }
    virtual void resize(int lines, int columns, int y, int x);
    
    // the following are inherited from EditBoxStatusListener

    virtual void on_wrap_change() {
	request_update(rgnIndicators);
    }
    
    virtual void on_dir_algo_change() {
	request_update(rgnIndicators);
    }
    
    virtual void on_selection_change() {
	request_update(rgnIndicators);
    }
    
    virtual void on_alt_kbd_change() {
	request_update(rgnIndicators);
    }
    
    virtual void on_auto_indent_change() {
	request_update(rgnIndicators);
    }

    virtual void on_auto_justify_change() {
	request_update(rgnIndicators);
    }

    virtual void on_modification_change() {
	request_update(rgnIndicators);
    }
    
    virtual void on_formatting_marks_change() {
	request_update(rgnIndicators);
    }
    
    virtual void on_position_change() {
	if (cursor_position_report)
	    request_update(rgnCursorPos);
    }
    
    virtual void on_read_only_change() {
	request_update(rgnIndicators);
    }
    
    virtual void on_smart_typing_change() {
	request_update(rgnIndicators);
    }
    
    virtual void on_rtl_nsm_change() {
	request_update(rgnIndicators);
    }
    
    virtual void on_maqaf_change() {
	request_update(rgnIndicators);
    }
    
    virtual void on_translation_mode_change() {
	request_update(rgnIndicators);
    }
};

#endif

