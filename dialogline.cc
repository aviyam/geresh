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

#include <stdarg.h>

#include "dialogline.h"
#include "editor.h"
#include "inputline.h"
#include "question.h"
#include "dbg.h"

DialogLine::DialogLine(Editor *aApp)
{
    app = aApp;
    modal_widget = NULL;
    transient = false;
}

void DialogLine::resize(int lines, int columns, int y, int x)
{
    Label::resize(lines, columns, y, x);
    if (modal_widget)
	modal_widget->resize(lines, columns, y, x);
}

void DialogLine::show_message(const char *msg)
{
    set_text(msg);
    transient = true;
}

void DialogLine::show_error_message(const char *msg)
{
    set_text(msg);
    transient = false;
}

void DialogLine::show_message_fmt(const char *fmt, ...)
{
    u8string msg;
    va_list ap;
    va_start(ap, fmt);
    msg.vcformat(fmt, ap);
    va_end(ap);
    show_message(msg.c_str());
}

INTERACTIVE void DialogLine::layout_windows()
{
    app->layout_windows();
    // move the cursor back to the modal widget
    if (modal_widget)
	modal_widget->invalidate_view();
}

INTERACTIVE void DialogLine::refresh()
{
    app->refresh();
    // move the cursor back to the modal widget
    if (modal_widget)
	modal_widget->invalidate_view();
}

void DialogLine::clear_transient_message()
{
    if (!empty() && transient)
	set_text("");
}

INTERACTIVE void DialogLine::cancel_modal()
{
    modal_canceled = true;
}

// modalize() - does event pumping on some interactive widget.

void DialogLine::modalize(Widget *wgt)
{
    modal_widget = wgt;
    wgt->resize(window_height(), window_width(), window_begy(), window_begx());
    modal_canceled = false;
    while (wgt->is_modal() && !modal_canceled) {
	Event evt;
	wgt->update();
	doupdate();
	get_next_event(evt, wgt->wnd);
	if (!handle_event(evt))
	    wgt->handle_event(evt);
    }
    modal_widget = NULL;
    show_message(modal_canceled ? _("Canceled") : "");
}

bool DialogLine::ask_yes_or_no(const char *msg, bool *canceled)
{
    Question ipt(msg);
    modalize(&ipt);
    if (canceled)
    	*canceled = modal_canceled;
    return modal_canceled ? false : ipt.get_answer();
}

int DialogLine::get_number(const char *msg, int default_num, bool *canceled)
{
    u8string cstrnum;
    cstrnum.cformat("%d", default_num);
    unistring strnum;
    strnum.init_from_utf8(cstrnum.c_str());
    InputLine ipt(msg, strnum);
    modalize(&ipt);
    if (canceled)
    	*canceled = modal_canceled;
    if (modal_canceled) {
	return 0;
    } else {
	cstrnum.init_from_unichars(ipt.get_text());
	return atoi(cstrnum.c_str());
    }
}

unistring DialogLine::query(const char *msg, const char *default_text,
			    int history_set, InputLine::CompleteType complete,
			    bool *alt_kbd)
{
    unistring text;
    text.init_from_utf8(default_text);
    return query(msg, text, history_set, complete, alt_kbd);
}

unistring DialogLine::query(const char *msg, const unistring &default_text,
			    int history_set, InputLine::CompleteType complete,
			    bool *alt_kbd)
{
    InputLine ipt(msg, default_text, history_set, complete);
    if (alt_kbd)
	ipt.set_alt_kbd(*alt_kbd);
    modalize(&ipt);
    if (alt_kbd)
	*alt_kbd = ipt.get_alt_kbd();
    return modal_canceled ? unistring() : ipt.get_text();
}

void DialogLine::update()
{
    if (modal_widget)
	modal_widget->update();
    else
	Label::update();
}

// immediate_update() - called when we want to update the terminal
// immediately, without having to wait for the event pump. As an intentional
// side effect, the terminal cursor moves to this window.
// TODO: move to the Widget class?

void DialogLine::immediate_update()
{
    if (terminal::is_interactive()) {
	update();
	doupdate();
    }
}

