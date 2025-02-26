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

#ifndef BDE_TYPES_H
#define BDE_TYPES_H

#include <string>
#include <stdarg.h> // u8string::cformat

#include <fribidi/fribidi.h>

#include "directvect.h"

typedef FriBidiChar	unichar;
typedef FriBidiStrIndex idx_t;

class unistring;

class u8string : public std::string {

public:

    u8string() { }
    u8string(const u8string &s, size_type pos = 0, size_type n = npos)
	: std::string(s, pos, n) { }
    u8string(const char *s)
	: std::string(s) { }
    u8string(const char *first, const char *last)
	: std::string(first, last) { }
    void init_from_unichars(const unichar *src, int len);
    void init_from_unichars(const unistring &str);
    u8string(const unistring &str) {
	init_from_unichars(str);
    }
    int len() const { return (int)size(); }
    void cformat(const char *fmt, ...);
    void vcformat(const char *fmt, va_list ap);
    void clear() {
	erase(begin(), end());
    }
    int index(char c, int from = 0) const {
	size_type pos = find_first_of(c, from);
	return pos == npos ? -1 : (int)pos;
    }
    int rindex(char c) const {
	size_type pos = find_last_of(c);
	return pos == npos ? -1 : (int)pos;
    }
    int index(const char *s, int from = 0) const;
    u8string substr(int from, int len = -1) const {
	if (len == -1 || (from + len > (int)size()))
	    len = size() - from;
	return u8string(&*(begin() + from), &*(begin() + from + len));
    }
    u8string erase_char(char xch) const;
    u8string trim() const;
    void inplace_trim();
    u8string toupper_ascii() const;
};

// we use "cstring" in the source code when we want to
// emphasize that this is not a UTF-8 encoded string.
#define cstring u8string

class unistring : public DirectVector<unichar> {

public:

    unistring() { }
    unistring(size_type n)
	: DirectVector<unichar>(n) { }
    unistring (const unichar *first, const unichar *last) {
	insert(end(), first, last);
    }

    void init_from_utf8(const char *s, int len);
    void init_from_utf8(const char *first, const char *last) {
	init_from_utf8(first, last - first);
    }
    void init_from_utf8(const char *s);
    unistring(const u8string &u8) {
	init_from_utf8(u8.c_str());
    }
    void init_from_latin1(const char *s);
    void init_from_filename(const char *filename);
    idx_t len() const { return (idx_t)size(); }

    void append(const unichar *s, size_type len) {
	insert(end(), s, s + len);
    }
    void append(const unistring &s, size_type len) {
	insert(end(), s.begin(), s.begin() + len);
    }
    void append(const unistring &s) {
	insert(end(), s.begin(), s.end());
    }
    void erase_head(size_type len) {
	erase(begin(), begin() + len);
    }
    unistring substr(int from, int len = -1) const {
	if (len == -1 || (from + len > (int)size()))
	    len = size() - from;
	return unistring(begin() + from, begin() + from + len);
    }
    unistring toupper_ascii() const;
    int index(unichar ch) const;
    bool has_char(unichar ch) const;
    int index(const unistring &sub, int from = 0) const;
};

#define STREQ(a,b) (strcmp(a, b) == 0)

#endif

