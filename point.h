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

#ifndef BDE_POINT_H
#define BDE_POINT_H

// struct Point represents a point in the EditBox buffer. It consists of a
// paragraph number and a character index within this paragraphs. The cursor
// itself is of type Point.

struct Point {

    int para;
    idx_t pos;

    Point(int para, idx_t pos) {
	this->para = para;
	this->pos = pos;
    }
    
    Point() {
	zero();
    }

    void zero() {
	para = 0;
	pos = 0;
    }

    void swap(Point &p) {
	Point tmp = p;
	p = *this;
	*this = tmp;
    }

    bool operator< (const Point &p) const {
	return para < p.para || (para == p.para && pos < p.pos);
    }
    bool operator== (const Point &p) const {
	return para == p.para && pos == p.pos;
    }
    bool operator> (const Point &p) const {
	return !(*this == p || *this < p);
    }
};

#endif

