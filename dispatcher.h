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

#ifndef BDE_DISPATCHER_H
#define BDE_DISPATCHER_H

#include "event.h"
#include <cstring>

// class Dispatcher represents a class that receives GUI events.

#define INTERACTIVE

struct binding_entry {
    Event evt;
    const char *action;
};

class Dispatcher {
    
public:

    Dispatcher() { }

    virtual bool do_action(const char *action) {
	return false;
    }

    virtual const char *get_event_action(const Event &evt) {
	return NULL;
    }
    
    virtual bool get_action_event(const char *action, Event &evt) {
	return false;
    }
    
    virtual const char *get_action_description(const char *action) {
	return NULL;
    }

    virtual bool handle_event(const Event &evt) {
	return do_action(get_event_action(evt));
    }
};

// :TODO: I should use STL's map instead of a simple linear search.

#define HAS_ACTIONS_MAP(CLASS, PARENT_CLASS)    \
    typedef void (CLASS::*method_ptr)();    \
    struct action_entry {		    \
	const char *action;		    \
	method_ptr method;		    \
	const char *short_description;	    \
    };					    \
    static action_entry actions_table[];    \
    virtual bool do_action(const char *action) {    \
	if (!action) return false;		    \
	action_entry *entry = actions_table;	    \
	while (entry->action) {			    \
	    if (!strcmp(entry->action, action)) {   \
		(this->*(entry->method))();	    \
		return true;			    \
	    }					    \
	    entry++;				    \
	}					    \
	return PARENT_CLASS::do_action(action);	    \
    }						    \
    virtual const char *get_action_description	    \
    (const char *action) {			    \
	if (!action) return NULL;		    \
	action_entry *entry = actions_table;	    \
	while (entry->action) {			    \
	    if (!strcmp(entry->action, action))	    \
		return entry->short_description;    \
	    entry++;				    \
	}					    \
	return PARENT_CLASS::			    \
	    get_action_description(action);	    \
    }

#define ADD_ACTION(CLASS, METHOD, DESC)	\
 { #METHOD, &CLASS::METHOD, DESC }

#define END_ACTIONS \
 { 0, 0 }

#define HAS_BINDINGS_MAP(CLASS, PARENT_CLASS)		    	\
    static binding_entry bindings_table[];		    	\
    virtual const char *get_event_action(const Event &evt) {	\
	binding_entry *entry = bindings_table;			\
	while (entry->action) {					\
	    if (entry->evt == evt)				\
		return entry->action;				\
	    entry++;						\
	}							\
	return PARENT_CLASS::get_event_action(evt);		\
    } \
    virtual bool get_action_event(const char *action, Event &evt) { \
	binding_entry *entry = bindings_table;			\
	while (entry->action) {					\
	    if (!strcmp(entry->action, action))	{		\
		evt = entry->evt;				\
		return true;					\
	    }							\
	    entry++;						\
	}							\
	return PARENT_CLASS::get_action_event(action, evt);	\
    }

#define END_BINDINGS \
 { Event(), 0 }

#endif

