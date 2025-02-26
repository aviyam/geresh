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

#ifndef BDE_SCROLLBAR_H
#define BDE_SCROLLBAR_H

#include "types.h"
#include "widget.h"

class Scrollbar : public Widget {

protected:

    int total_size;
    int page_size;
    int page_pos;
    bool dirty;

public:

    Scrollbar();
    void set_total_size(int sz);
    void set_page_size(int sz);
    void set_page_pos(int pos);
    
    virtual void update();
    virtual void invalidate_view() { dirty = true; }
    virtual bool is_dirty() const { return dirty; }
    virtual void resize(int lines, int columns, int y, int x);
};

#endif

