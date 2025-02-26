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

#ifndef BDE_TERMINAL_H
#define BDE_TERMINAL_H

#include <config.h>

// _XOPEN_SOURCE_EXTENDED is necessary for the wide-curses API
#ifdef HAVE_WIDE_CURSES
# define _XOPEN_SOURCE_EXTENDED 1
#endif
 
#if defined(HAVE_NCURSESW_NCURSES_H)
# include <ncursesw/ncurses.h>
#elif defined(HAVE_NCURSES_H)
# include <ncurses.h>
#else
# include <curses.h>
#endif

// :TODO: move this gettext() stuff to a more relevant header?
#ifdef USE_GETTEXT
# include <libintl.h>
# define _(String) gettext(String)
# define gettext_noop(String) String
# define N_(String) gettext_noop(String)
#else
# define _(String) String
# define N_(String) String
# define textdomain(Domain)
# define bindtextdomain(Package, Directory)
#endif

class terminal {
private:
    static bool initialized;

public:
    static bool is_utf8;	// Are we in UTF-8 locale?

    static bool force_iso88598;	// Assume this is an ISO-8859-8 terminal?

    static bool is_fixed;	// Do we use a "fixed" terminal? a terminal
				// which is not capable of displaying wide-
				// characetrs and combining characters.

    static bool is_color;	// Does our terminal support colors?
    
    static bool use_default_colors;// ncurses' use_default_colors() used?

    static bool do_arabic_shaping; // instruct all widgets to do arabic shaping.

    static bool graphical_boxes;   // Use graphical chars for the menu, scrollbar.

    static void init();
    static void finish();
    static bool was_ctrl_c_pressed();
    static bool is_interactive();
    static void determine_locale();
    static bool under_x11();
};

void DISABLE_SIGTSTP();

#endif

