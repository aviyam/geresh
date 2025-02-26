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

#ifndef BDE_DIALOGLINE_H
#define BDE_DIALOGLINE_H

#include "label.h"
#include "inputline.h"

class Editor;

// DialogLine does the dialog with the user: it displays informative
// and error messages, asks for input and for confirmations.

class DialogLine : public Label {

    Editor *app;
    Widget *modal_widget;
    bool modal_canceled;

    // A DialogLine can display two kinds of messages: transient and
    // non-transient. Transient messages are displayed for a brief time,
    // whereas non-transient messages are durable. The latter are used for
    // error messages.
    bool transient;

    void modalize(Widget *wgt);

public:

    HAS_ACTIONS_MAP(DialogLine, Label);
    HAS_BINDINGS_MAP(DialogLine, Label);
    
    DialogLine(Editor *aApp);

    void show_message(const char *msg);
    void show_message_fmt(const char *fmt, ...);
    void show_error_message(const char *msg);
    void clear_transient_message();
   
    unistring query(const char *msg, const char *default_text = NULL,
		    int history_set = 0,
		    InputLine::CompleteType complete = InputLine::cmpltOff,
		    bool *alt_kbd = NULL);
    unistring query(const char *msg, const unistring &default_text,
		    int history_set = 0,
		    InputLine::CompleteType complete = InputLine::cmpltOff,
		    bool *alt_kbd = NULL);

    int get_number(const char *msg, int default_num, bool *canceled = NULL);
    bool ask_yes_or_no(const char *msg, bool *canceled = NULL);
    
    INTERACTIVE void cancel_modal();
    INTERACTIVE void layout_windows();
    INTERACTIVE void refresh();

    virtual void update();
    void immediate_update();
    virtual void resize(int lines, int columns, int y, int x);
};

#endif

