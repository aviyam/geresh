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

#ifndef BDE_DBG_H
#define BDE_DBG_H

#include <config.h>

void set_debug_level(int level);
bool print_debug_level(int level);
void debug_print(const char *fmt, ...);
void fatal(const char *fmt, ...);

#ifdef DEBUG
# define DBG(level, args)	\
    do { \
	if (print_debug_level(level)) \
	    debug_print args; \
    } while (0)
#else
# define DBG(level, args)	\
    do { ; } while (0)
#endif

#endif

