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

#include "scrollbar.h"
#include "themes.h"

Scrollbar::Scrollbar()
{
    create_window();
    scrollok(wnd, 0);
    total_size = 0;
    page_size  = 0;
    page_pos   = 0;
    dirty = true;
}

void Scrollbar::set_page_size(int sz)
{
    if (sz != page_size) {
	page_size = sz;
	invalidate_view();
    }
}

void Scrollbar::set_total_size(int sz)
{
    if (sz != total_size) {
	total_size = sz;
	invalidate_view();
    }
}

void Scrollbar::set_page_pos(int pos)
{
    if (pos != page_pos) {
	page_pos = pos;
	invalidate_view();
    }
}

void Scrollbar::update()
{
    if (!dirty)
	return;

    int thumb_pos = 0;
    if (page_pos > 0)
	thumb_pos = (int)((double(page_pos)/double(total_size))*window_height());
    int thumb_size = (int)((double)page_size/double(total_size)*window_height()+1);
    if (thumb_size > window_height())
	thumb_size = window_height();
    if (page_size >= total_size) // don't show if we don't have a reason
	thumb_size = 0;
    if (thumb_size == window_height())
	thumb_size--;
    if (page_pos + page_size >= total_size)
	thumb_pos = window_height() - thumb_size;
    
    werase(wnd);
    attribute_t base_attr  = get_attr(SCROLLBAR_ATTR);
    attribute_t thumb_attr = get_attr(SCROLLBAR_THUMB_ATTR);
    for (int i = 0; i < window_height(); i++) {
	if (i >= thumb_pos && i < thumb_pos + thumb_size) {
	    wattrset(wnd, thumb_attr);
	    mvwhline(wnd, i, 0, ' ', 1);
	} else {
	    wattrset(wnd, base_attr);
	    mvwhline(wnd, i, 0, terminal::graphical_boxes ? ACS_VLINE : '|', 1);
	}
    }    

    wnoutrefresh(wnd);
    dirty = false;
}

void Scrollbar::resize(int lines, int columns, int y, int x)
{
    Widget::resize(lines, columns, y, x);
    invalidate_view();
}

