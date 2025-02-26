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

#ifndef MY_WCTOB
#define MY_WCTOB

#include <config.h>

// BTOWC and WCTOB are used for terminal encoding and we assume an iso-8859-8
// encoding if the system lacks them.

#if defined(HAVE_WCTOB) || defined(HAVE_BTOWC)
#  include <wchar.h>
#endif

#include "iso88598.h"

#if defined(HAVE_BTOWC)
#  define BTOWC(b) btowc(b)
#else
#  define BTOWC(b) iso88598_to_unicode(b)
#endif

#if defined(HAVE_WCTOB)
#  define WCTOB(w) wctob(w)
#else
#  define WCTOB(w) unicode_to_iso88598(w)
#endif

#endif

