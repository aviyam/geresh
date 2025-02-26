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

#ifndef BDE_LABEL_H
#define BDE_LABEL_H

#include "types.h"
#include "widget.h"

// Label is a simple widget that displays a string

class Label : public Widget {

    u8string text;
    bool dirty;
    bool is_highlighted;

public:

    Label();
    Label(const char *aText);

    void set_text(const char *aText);
    void highlight(bool value = true);
    bool empty() const { return (text.size() == 0); }
    virtual void update();
    virtual void invalidate_view() { dirty = true; }
    virtual bool is_dirty() const { return dirty; }
    virtual void resize(int lines, int columns, int y, int x);
};

#endif

