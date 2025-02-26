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

#ifndef BDE_WIDGET_H
#define BDE_WIDGET_H

#include "dispatcher.h"
#include "terminal.h"

typedef int attribute_t;

class Widget : public Dispatcher {

    bool modal;

public:

    WINDOW *wnd; // public

    Widget();
    virtual ~Widget();

    bool create_window(int lines = 1, int cols = 5);

    void destroy_window() {
	if (wnd) {
	    delwin(wnd);
	    wnd = NULL;
	}
    }

    bool is_valid_window() const {
	return wnd != NULL;
    }

    void enable_keypad() {
	if (is_valid_window())
	    keypad(wnd, TRUE);
    }

    int window_width() const {
	int x, y;
	getmaxyx(wnd, y, x);
	return x;
    }

    int window_height() const {
	int x, y;
	getmaxyx(wnd, y, x);
	return y;
    }

    int window_begx() const {
	int x, y;
	getbegyx(wnd, y, x);
	return x;
    }

    int window_begy() const { 
	int x, y;
	getbegyx(wnd, y, x);
	return y;
    }
  
    void put_unichar(unichar ch, unichar bad_repr);
    void draw_string(const char *u8, bool align_right = false);

    virtual void resize(int lines, int columns, int y, int x);
    virtual void update() = 0;
    virtual bool is_dirty() const = 0;
    virtual void invalidate_view() = 0;

    virtual void end_modal() { modal = false; }
    virtual void set_modal() { modal = true; }
    virtual bool is_modal() const { return modal; }
    
    static void signal_error();
};

#endif

