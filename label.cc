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

#include "label.h"
#include "themes.h"

Label::Label()
{
    create_window();
    is_highlighted = false;
    dirty = true;
}

Label::Label(const char *aText)
{
    create_window();
    is_highlighted = false;
    set_text(aText);
}

void Label::set_text(const char *aText)
{
    text = aText ? aText : "";
    // we append one space so that if this widget gets the focus,
    // the terminal cursor won't stick to the text. this is for
    // aesthetic only.
    text += ' ';
    dirty = true;
}

void Label::highlight(bool value)
{
    is_highlighted = value;
    invalidate_view();
}

void Label::update()
{
    if (!dirty)
	return;
    wbkgd(wnd, get_attr(is_highlighted ? STATUSLINE_ATTR : DIALOGLINE_ATTR));
    wmove(wnd, 0, 0);
    wclrtoeol(wnd);
    draw_string(text.c_str(), true);
    wnoutrefresh(wnd);
    dirty = false;
}

void Label::resize(int lines, int columns, int y, int x)
{
    Widget::resize(lines, columns, y, x);
    dirty = true;
}

