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

#ifndef BDE_QUESTION_H
#define BDE_QUESTION_H

#include "label.h"

// Question is a simple modal widget that displays a y/n question and waits
// for an answer. We use gettext() to retrieve the possible "y"es and "n"o
// characters.

class Question : public Label {

    bool positive_answer;
    unistring yes_chars;    // string of localized "y"es characters
    unistring no_chars;	    // string of localized "n"o characters

public:

    Question(const char *aText);
    virtual bool handle_event(const Event &evt);
    bool get_answer() const { return positive_answer; }
};

#endif

