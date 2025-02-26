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
#include <stdio.h>
#include <stdlib.h>   // exit()

#include "terminal.h" // for the gettext stuff
#include "dbg.h"

static int debug_level = 0;

void set_debug_level(int level)
{
    debug_level = level;
}

bool print_debug_level(int level)
{
    return level <= debug_level;
}

void debug_print(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

// I've just noticed that my fatal() function is very similar to fribidi's
// die(). Strange. I've taken the variable names from an example in K&R, and
// used the output of 'ls' ("try ... for more info"). Perhaps it proves
// that two monkeys sitting at two keyboards and not typing randomly can
// produce the same program :-)

void fatal(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (fmt) {
	fprintf(stderr, PACKAGE ": ");
	vfprintf(stderr, fmt, ap);
    }
    fprintf(stderr, _("Try `%s --help' for more information.\n"), PACKAGE);
    va_end(ap);
    exit(1);
}

