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

#include "question.h"

Question::Question(const char *aText)
    : Label(aText)
{
    yes_chars.init_from_utf8(_("yY"));
    no_chars.init_from_utf8(_("nN"));
    set_modal();
    positive_answer = false;
}

bool Question::handle_event(const Event &evt)
{
    if (evt.is_literal()) {
	if (yes_chars.has_char(evt.ch)) {
	    positive_answer = true;
	    end_modal();
	    return true;
	}
	if (no_chars.has_char(evt.ch)) {
	    positive_answer = false;
	    end_modal();
	    return true;
	}
    }
    return Label::handle_event(evt);
}

