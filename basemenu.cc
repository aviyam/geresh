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

#include "basemenu.h"
#include "themes.h"
#include <cstring>

///////////////////////////// PopupMenu ////////////////////////////////////

PopupMenu::PopupMenu(PopupMenu *aParent, PulldownMenu aMnu)
{
    parent = aParent;
    init(aMnu);
}

void PopupMenu::init(PulldownMenu aMnu)
{
    mnu     = aMnu;
    count   = 0;
    top     = 0;
    current = 0;
    if (aMnu) {
	complete_menu(mnu);
	while (mnu[count].action)
	    count++;
    }
}

void PopupMenu::complete_menu(PulldownMenu mnu)
{
    for (int i = 0; mnu[i].action; i++) {
	if (get_primary_target()->get_action_event(mnu[i].action, mnu[i].evt)) {
	    if (!mnu[i].desc)
		mnu[i].desc = get_primary_target()->get_action_description(mnu[i].action);
	} else {
	    if (get_secondary_target()) {
		get_secondary_target()->get_action_event(mnu[i].action, mnu[i].evt);
		if (!mnu[i].desc)
		    mnu[i].desc = get_secondary_target()->get_action_description(mnu[i].action);
	    }
	}
	if (mnu[i].label) {
	    unistring u = u8string(mnu[i].label);
	    if (u.has_char('~')) {
		mnu[i].shortkey = u[u.index('~') + 1];
		if (mnu[i].shortkey >= 'A' && mnu[i].shortkey <= 'Z')
		    mnu[i].shortkey -= 'A' - 'a';
	    }
	}
    }
}

INTERACTIVE void PopupMenu::move_previous_item()
{
    if (current > 0)
	while (mnu[--current].action[0] == '-')
	    ;
    else
	move_last_item();
    
    if (current < top)
	top = current;
}

INTERACTIVE void PopupMenu::move_next_item()
{
    if (current < count - 1)
	while (mnu[++current].action[0] == '-')
	    ;
    else
	move_first_item();

    if (current >= top + window_height() - 2)
	top = current - window_height() + 3;
}

INTERACTIVE void PopupMenu::move_first_item()
{
    current = top = 0;
}

INTERACTIVE void PopupMenu::move_last_item()
{
    current = count > 1 ? count - 2 : 0;
    move_next_item(); // update top
}

void PopupMenu::end_modal(PostResult rslt)
{
    post_result = rslt;
    Widget::end_modal();
}

INTERACTIVE void PopupMenu::prev_menu()
{
    end_modal(mnuPrev);
}

INTERACTIVE void PopupMenu::next_menu()
{
    if (mnu[current].submenu)
	select();
    else
	end_modal(mnuNext);
}

void PopupMenu::update_ancestors()
{
    if (parent) {
	parent->update_ancestors();
	parent->update();
    }
}

INTERACTIVE void PopupMenu::select()
{
    if (mnu[current].command_parameter1 || mnu[current].command_parameter2) {
	show_hint(""); // clear the dialogline _before_
		       // executing the command (so we won't erase
		       // what the command prints there).
	do_command(mnu[current].command_parameter1,
		   mnu[current].command_parameter2,
		   mnu[current].command_parameter3);
    }

    if (!mnu[current].submenu) {
	// If it's not a submenu then it's easy.
	if (mnu[current].evt.empty()) {
	    // When the event is not set, it usualy means that
	    // do_command() was executed and no more processing
	    // is required.
	    end_modal(mnuCancel);
	} else {
	    set_next_event(mnu[current].evt);
	    end_modal(mnuSelect);
	    show_hint("");
	}
	return;
    }

    // No, we need to post a submenu.
    PopupMenu::PostResult post_result;
    Event evt;
    PopupMenu *p = create_popupmenu(this, mnu[current].submenu);
    post_result = p->post(window_begx() + window_width() - 2,
	    window_begy() + current - top + 1, evt);
    delete p;

    switch (post_result) {
    case PopupMenu::mnuSelect:
	end_modal(mnuSelect);
	break;
    case PopupMenu::mnuCancel:
	end_modal(mnuCancel);
	break;
    case PopupMenu::mnuNext:
	end_modal(mnuNext);
	break;
    case PopupMenu::mnuPrev:
	clear_other_popups();
	update_ancestors();
	break;
    }
}

INTERACTIVE void PopupMenu::cancel_menu()
{
    end_modal(mnuCancel);
    show_hint("");
}

bool PopupMenu::handle_event(const Event &evt)
{
    if (evt.is_literal()) {
	for (int i = 0; i < count; i++) {
	    if (mnu[i].shortkey == evt.ch) {
		current = i;
		update();
		select();
		return true;
	    }
	}
    }
    return Dispatcher::handle_event(evt);
}

void PopupMenu::draw_frame()
{
    int attr = get_attr(MENU_FRAME_ATTR);
    if (terminal::graphical_boxes) {
	wborder(wnd, ACS_VLINE|attr, ACS_VLINE|attr, ACS_HLINE|attr, ACS_HLINE|attr,
		     ACS_ULCORNER|attr, ACS_URCORNER|attr, ACS_LLCORNER|attr, ACS_LRCORNER|attr);
    } else {
	wborder(wnd, '|'|attr, '|'|attr, '-'|attr, '-'|attr,
		     '+'|attr, '+'|attr, '+'|attr, '+'|attr);
    }

    if (top != 0)
	mvwhline(wnd, 0, 2, terminal::graphical_boxes ? ACS_UARROW : '^', 1);
    if (top + window_height() - 2 < count)
	mvwhline(wnd, window_height() - 1, 2, terminal::graphical_boxes ? ACS_DARROW : 'v', 1);
}

int PopupMenu::get_item_optimal_width(int item)
{
    if (mnu[item].label)
	return strlen(mnu[item].label) + 1 + mnu[item].evt.to_string().length() + 2;
    else
	return 2;
}

int PopupMenu::get_optimal_width()
{
    int max = 0;
    for (int i = 0; i < count; i++) {
	int w = get_item_optimal_width(i);
	if (w > max)
	    max = w;
    }
    return max;
}

void PopupMenu::update()
{
    // Draw the menu.

    int last_visible_item = top + window_height() - 3;
    if (last_visible_item >= count)
	last_visible_item = count - 1;
    int item;

    // Draw the labels
    for (item = top; item <= last_visible_item; item++) {
	int y = item - top + 1;
	wattrset(wnd, (item == current) ? get_attr(MENU_SELECTED_ATTR) : get_attr(MENU_ATTR));
	wmove(wnd, y, 1);
	for (int i = 0; i < window_width() - 2; i++)
	    waddch(wnd, ' ');
	wmove(wnd, y, 2);
	
	if (mnu[item].action[0] != '-') {
	    u8string u8key;
	    unistring ulabel = u8string(mnu[item].label);

	    int key_ofs = 0;
	    if (mnu[item].shortkey) {
		key_ofs = ulabel.index('~')+1;
		unichar key = ulabel[key_ofs];
		u8key = u8string(unistring(&key, &key + 1));
		ulabel.erase(ulabel.begin()+key_ofs-1, ulabel.begin()+key_ofs);
	    }
    
	    u8string label_without_tilde(ulabel);
	    draw_string(label_without_tilde.c_str());
	    
	    if (mnu[item].shortkey) {
		wmove(wnd, y, key_ofs+1);
		wattrset(wnd, (item == current) ? get_attr(MENU_LETTER_SELECTED_ATTR) : get_attr(MENU_LETTER_ATTR));
		draw_string(u8key.c_str());
	    }

	    wattrset(wnd, (item == current) ? get_attr(MENU_INDICATOR_SELECTED_ATTR) : get_attr(MENU_INDICATOR_ATTR));
	    if (mnu[item].state_id) {
		wmove(wnd, y, 1);
		int id = mnu[item].state_id;
		bool checked = get_item_state(id);
		if (checked || id < 5000) { // Avoid painting a blank (" ")
					    // in order not to affect
					    // the cursor color.
		    draw_string(checked
				    ? (id >= 5000 ? "*" : "+")
				    : (id >= 5000 ? " " : "-"));
		}
	    }
	    wattrset(wnd, (item == current) ? get_attr(MENU_SELECTED_ATTR) : get_attr(MENU_ATTR));

	    if (mnu[item].submenu) {
		wmove(wnd, y, window_width()-3);
		draw_string(">");
	    } else {
		// Print the hot-key
		u8string keyname = mnu[item].evt.to_string();
		wmove(wnd, y, window_width() - keyname.length() - 3);
		waddch(wnd, ' ');		
		draw_string(keyname.c_str());
		waddch(wnd, ' ');		
	    }
	}
    }

    // Draw the frame
    wattrset(wnd, get_attr(MENU_FRAME_ATTR));
    draw_frame();
    for (item = top; item <= last_visible_item; item++) {
	int y = item - top + 1;
	if (mnu[item].action[0] == '-') {
	    if (terminal::graphical_boxes) {
		mvwhline(wnd, y, 0, ACS_LTEE, 1);
		mvwhline(wnd, y, 1, ACS_HLINE, window_width()-2);
		mvwhline(wnd, y, window_width()-1, ACS_RTEE, 1);
	    } else {
		mvwhline(wnd, y, 2, '-', window_width()-4);
	    }
	}
    }
    
    // Place the cursor before the item (useful for monochrome terminals).
    wmove(wnd, current - top + 1, 1);

    show_hint(mnu[current].desc);

    wnoutrefresh(wnd);
}

// reposition() - adjusts the window size to fit the screen
//                and the menu items.

void PopupMenu::reposition(int x, int y)
{
    int width  = get_optimal_width() + 2;
    int height = count + 2;

    int scr_width, scr_height;
    getmaxyx(stdscr, scr_height, scr_width);
   
    if (x + width > scr_width) {
	x = scr_width - width;
	if (x < 0)
	    x = 0;
	if (width > scr_width)
	    width = scr_width;
    }

    if (y + height > scr_height) {
	y = scr_height - height;
	if (y < 0) {
	    y = 0;
	    height = scr_height;
	}
	// don't hide the menubar line
	if (y == 0 && scr_height > 3) {
	    y++;
	    height--;
	}
    }

    resize(height, width, y, x);
}

PopupMenu::PostResult PopupMenu::post(int x, int y, Event &evt)
{
    if (!is_valid_window()) // if we haven't yet created it
	create_window(1, 1);
    scrollok(wnd, 0); // so we can draw '+' at the bottom right corner.
    reposition(x, y);
    set_modal();
    while (is_modal()) {
	Event evt;
	update();
	doupdate();
	get_next_event(evt, wnd);
	handle_event(evt);
    }
    //show_hint(""); // I moved it to a better place because I don't
		     // want to erase what do_command might print there.
    destroy_window();
    return post_result;
}

INTERACTIVE void PopupMenu::screen_resize()
{
#ifdef KEY_RESIZE
    set_next_event(Event(KEY_RESIZE));
    cancel_menu();
#endif
}

/////////////////////////////// Menubar ////////////////////////////////////

Menubar::Menubar()
{
    create_window(1, 1);
}

void Menubar::init(MenubarMenu aMnu)
{
    mnu = aMnu;
    count = 0;
    while (mnu[count].label)
	count++;
    current = -1;
    dirty = true;
}

void Menubar::set_current(int i)
{
    current = i;
    invalidate_view();
}

INTERACTIVE void Menubar::select()
{
    PopupMenu::PostResult post_result;
    Event evt;

    PopupMenu *p = create_popupmenu(mnu[current].submenu);
    post_result = p->post(get_ofs(current), 1, evt);
    delete p;

    if (post_result == PopupMenu::mnuCancel || post_result == PopupMenu::mnuSelect) {
	set_current(-1);
    }
    refresh_screen();

    switch (post_result) {
    case PopupMenu::mnuCancel:
	doupdate();
	end_modal();
	break;
    case PopupMenu::mnuSelect:
	end_modal();
	break;
    case PopupMenu::mnuPrev:
	prev_menu();
	update();
	select();
	break;
    case PopupMenu::mnuNext:
	next_menu();
	update();
	select();
	break;
    }

}

INTERACTIVE void Menubar::next_menu()
{
    if (current >= count - 1)
	set_current(0);
    else
	set_current(current + 1);
}

INTERACTIVE void Menubar::prev_menu()
{
    if (current <= 0)
	set_current(count - 1);
    else
	set_current(current - 1);
}

int Menubar::get_ofs(int item)
{
    int ofs = 1;
    for (int i = 0; i < item; i++)
	ofs += strlen(mnu[i].label) + 2;
    return ofs;
}

void Menubar::resize(int lines, int columns, int y, int x)
{
    Widget::resize(lines, columns, y, x);
    invalidate_view();
}

INTERACTIVE void Menubar::screen_resize()
{
#ifdef KEY_RESIZE
    set_next_event(Event(KEY_RESIZE));
    end_modal();
#endif
}

void Menubar::update()
{
    if (!dirty)
	return;

    werase(wnd);
    wattrset(wnd, get_attr(MENUBAR_ATTR));
    wmove(wnd, 0, 0);
    for (int i = 0; i < window_width(); i++)
	waddch(wnd, ' ');
    wmove(wnd, 0, 0);

    for (int i = 0; i < count; i++) {
	wmove(wnd, 0, get_ofs(i));
	wattrset(wnd, (current == i) ? get_attr(MENU_SELECTED_ATTR) : get_attr(MENUBAR_ATTR));
	draw_string(current == i ? " " : " ", false);
	draw_string(mnu[i].label, false);
	draw_string(current == i ? " " : " ", false);
    }

    // Place the cursor before the item (useful for monochrome terminals).
    if (current >= 0)
	wmove(wnd, 0, get_ofs(current));

    wnoutrefresh(wnd);
    dirty = false;
}

bool Menubar::handle_event(const Event &evt)
{
    if (evt.is_literal()) {
	for (int i = 0; i < count; i++) {
	    if (mnu[i].label) {
		unistring u = u8string(mnu[i].label);
		unichar shortkey = u[0];
		if (shortkey >= 'A' && shortkey <= 'Z')
		    shortkey -= 'A' - 'a';
		if (shortkey == evt.ch) {
		    set_current(i);
		    update();
		    select();
		    return true;
		}
	    }
	}
    }
    return Dispatcher::handle_event(evt);
}

void Menubar::exec()
{
    set_current(0);
    set_modal();
    while (is_modal()) {
	Event evt;
	update();
	doupdate();
	get_next_event(evt, wnd);
	handle_event(evt);
    }
    set_current(-1);
    update();
}

