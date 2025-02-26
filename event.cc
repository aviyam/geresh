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

#include "event.h"
#include "my_wctob.h"
#include "dbg.h"

static const char *ext_keyname(int keycode)
{
    switch (keycode) {
    case KEY_DOWN:  return "Down";
    case KEY_UP:    return "Up";
    case KEY_LEFT:  return "Left";
    case KEY_RIGHT: return "Right";
    case KEY_HOME:  return "Home";
    case KEY_END:   return "End";
    case KEY_BACKSPACE: return "BkSp";
    case KEY_DC:    return "Del";
    case KEY_IC:    return "Insert";
    case KEY_NPAGE: return "PgDn";
    case KEY_PPAGE: return "PgUp";
    default: return keyname(keycode);
    }
}

u8string Event::to_string() const
{
    u8string s;
    if ((modifiers & VIRTUAL) || (keycode == 0 && !ch)) {
	// No actual event
	s = "";
    } else if (is_literal()) {
	s = u8string(unistring(&ch, &ch + 1));
    } else {
	//DBG(2, ("comp: "));
	if (modifiers) {
	    if (modifiers & CTRL)
		s += "C-";
	    if (modifiers & ALT)
		s += "M-";
	    if (modifiers & SHIFT)
		s += "Shift-";
	}
	if (ch) {
	    if ((modifiers & ALT) && ch == '\t') {
		// Hmmm... this is a special case. Ctrl-I is actually TAB.
		return u8string("C-M-i");
	    }
	    if ((modifiers & CTRL) && ch == '^') {
		// Hmmm... this is a more friendly representation.
		return u8string("C-Spc");
	    }
	    s += u8string(unistring(&ch, &ch + 1)).c_str();
	} else {
	    if (keycode >= KEY_F(0) && keycode <= KEY_F(63)) {
		u8string num;
		num.cformat("F%d", keycode - KEY_F(0));
		s += num;
	    } else {
		s += ext_keyname(keycode);
	    }
	}
    }
    return s;
}

void Event::print()
{
    DBG(2, ("EVENT: "));
    if (is_literal())
	DBG(2, ("literal: '%lc'\n", ch));
    else {
	DBG(2, ("comp: "));
	if (modifiers) {
	    if (modifiers & CTRL)
		DBG(2, ("CTRL-"));
	    if (modifiers & ALT)
		DBG(2, ("ALT-"));
	    if (modifiers & SHIFT)
		DBG(2, ("SHIFT-"));
	}
	if (ch)
	    DBG(2, ("'%lc'\n", ch));
	else
	    DBG(2, ("%s\n", keyname(keycode)));
    }
}

#ifdef HAVE_WIDE_CURSES
static void low_level_get_wch(Event &evt, WINDOW *wnd)
{
    wint_t c;
    int ret;
    do {
	ret = wget_wch(wnd, &c);
	if (ret == KEY_CODE_YES) {
	    evt.ch = 0;
	    evt.keycode = (int)c;
	} else if (ret == OK) {
	    evt.keycode = 0;
	    evt.ch = (unichar)c;
	}
    } while (ret == ERR);
}
#else
static void low_level_get_wch(Event &evt, WINDOW *wnd)
{
    int ret;
    do {
	ret = wgetch(wnd);
	if (ret >= 256) {
	    evt.ch = 0;
	    evt.keycode = ret;
	} else {
	    evt.keycode = 0;
	    evt.ch = terminal::force_iso88598 ? iso88598_to_unicode(ret) : BTOWC(ret);
	}
    } while (ret == ERR);
}
#endif

static void get_base_event(Event &evt, WINDOW *wnd)
{
    evt.type = evtKbd;
    evt.modifiers = 0;
    low_level_get_wch(evt, wnd);
    if (evt.keycode == 0 && evt.ch < 32) {
	switch (evt.ch) {
	case 9:
	case 13:
	case 27:
	    // leave alone TAB, ENTER, ESC
	    break;
	case 0:
	case 28:
	case 29:
	case 30:
	case 31:
	    evt.ch += 'A' - 1;
	    evt.modifiers = CTRL;
	    break;
	default:
	    evt.ch += 'a' - 1;
	    evt.modifiers = CTRL;
	}
    }
}

bool is_event_pending = false;
Event pending_event;

void get_next_event(Event &evt, WINDOW *wnd)
{
    if (is_event_pending) {
	evt = pending_event;
	is_event_pending = false;
	return;
    }

    get_base_event(evt, wnd);
    if (evt.ch == 27) {
	// emulate ALT
	get_base_event(evt, wnd);
	if (evt.ch != 27) // ...but make sure we can still generate ESCape
	    evt.modifiers |= ALT;
    }
}

void set_next_event(const Event &evt)
{
    is_event_pending = true;
    pending_event = evt;
}

