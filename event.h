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

#ifndef BDE_EVENT_H
#define BDE_EVENT_H

#include "types.h"
#include "terminal.h"

#ifdef CTRL
# undef CTRL
#endif

#define SHIFT	1
#define CTRL	2
#define ALT	4
#define VIRTUAL 8

enum EventType { evtKbd, evtMouse };

class Event {
public:
   
    EventType type;
    
    int modifiers;
    unichar ch; 
    int keycode;

    bool operator== (const Event &other) const
    {
	return  type == other.type &&
		modifiers == other.modifiers &&
		ch == other.ch &&
		keycode == other.keycode;
    }

    Event() { }

    Event(int modifiers, unichar ch, int keycode = 0)
    {
	type = evtKbd;
	this->modifiers = modifiers;
	this->ch = ch;
	this->keycode = keycode;
    }

    Event(int keycode)
    {
	type = evtKbd;
	this->modifiers = 0;
	this->ch = 0;
	this->keycode = keycode;
    }

    bool is_literal() const
    {
	return (ch != 0 && ch != 27/*ESC*/ && modifiers == 0);
    }

    bool empty() const
    {
	return !modifiers && !ch && !keycode;
    }
	
    u8string to_string() const;

    void print();
};

void get_next_event(Event &evt, WINDOW *wnd);
void set_next_event(const Event &evt);

#endif

