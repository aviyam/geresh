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

#ifndef BDE_TRANSTBL_H
#define BDE_TRANSTBL_H

#include <map>

#include "types.h"

// The TranslationTable class matches one character with another. In other
// words, It's a hash (map) of characters.
//
// For example, the hebrew keyboard emulation is implemented as a
// TranslationTable that maps english characters to the hebrew characters
// that sit in their place on the keyboard.

class TranslationTable {
    
    std::map<unichar, unichar> charmap;
    
public:

    TranslationTable() { }
  
    bool empty() const { return charmap.empty(); }
    bool load(const char *);
    bool translate_char(unichar &ch) const;
};

#endif

