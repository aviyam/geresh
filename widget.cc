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

#include "widget.h"
#include "mk_wcwidth.h"
#include "bidi.h"
#include "shaping.h"
#include "my_wctob.h"

Widget::Widget()
{
    wnd = NULL;
    modal = false;
}

Widget::~Widget()
{
    destroy_window();
}

bool Widget::create_window(int lines, int cols)
{
    if (terminal::is_interactive()) {
	wnd = newwin(lines, cols, 0, 0);
	enable_keypad();
    }
    return wnd != NULL;
}

void Widget::resize(int lines, int columns, int y, int x)
{
    wresize(wnd, lines, columns);
    mvwin(wnd, y, x);
}

void Widget::put_unichar(unichar ch, unichar bad_repr)
{
#ifdef HAVE_WIDE_CURSES
    if (!terminal::is_utf8 && WCTOB(ch) == EOF)
	ch = bad_repr;
    waddnwstr(wnd, (wchar_t*)&ch, 1);
#else
    int ich = terminal::force_iso88598 ? unicode_to_iso88598(ch) : WCTOB(ch);
    if (ich == EOF)
	ich = bad_repr;
    waddch(wnd, (unsigned char)ich);
#endif
}

// draw_string() - draws a UTF-8 string. This is a very simple routine and
// it's used by the most simple widgets only (like Label).

void Widget::draw_string(const char *u8, bool align_right)
{
    unistring text, vis_text;
    text.init_from_utf8(u8);

    // trim string to fit window width
    int wnd_x, dummy;
    getyx(wnd, dummy, wnd_x);
    int swidth = wnd_x;
    unichar *trim_pos = text.begin();
    while (trim_pos < text.end()) {
	int char_width = mk_wcwidth(*trim_pos);
	if (swidth + char_width > window_width())
	    break;
	swidth += char_width;
	trim_pos++;
    }
    text.erase(trim_pos, text.end());
   
    // convert to visual
    direction_t dir = BiDi::determine_base_dir(text.begin(), text.size(),
					       algoUnicode);
    BiDi::simple_log2vis(text, dir, vis_text);

    if (terminal::do_arabic_shaping) {
	int new_len = shape(vis_text.begin(), vis_text.len());
	swidth -= vis_text.len() - new_len;
	vis_text.resize(new_len);
    }

    // draw the string
    if (align_right && dir == dirRTL)
	wmove(wnd, 0, window_width() - swidth);
    for (int i = 0; i < vis_text.len(); i++) {
	put_unichar(vis_text[i], '?');
    }

    // reposition the cursor
    if (align_right && dir == dirRTL)
	wmove(wnd, 0, window_width() - swidth - 1);
}

void Widget::signal_error()
{
    // I don't like those awful beeps,
    // but people may not be familiar with flashes.

    //flash();
    beep();
}

