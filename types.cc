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

#ifdef HAVE_VASPRINTF
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif
#endif
#include <string.h>	// strlen
#include <stdio.h>	// vsnprintf, vasprintf

#include <algorithm>	// find

#include "types.h"
#include "converters.h" // guess_encoding
#include "utf8.h"

void unistring::init_from_utf8(const char *s, int len)
{
    if (!s) {
	clear();
    } else {
	resize(len);
	int count = utf8_to_unicode(begin(), s, len);
	resize(count);
    }
}

void unistring::init_from_utf8(const char *s)
{
    if (!s)
	clear();
    else
	init_from_utf8(s, strlen(s));
}

void unistring::init_from_latin1(const char *s)
{
    clear();
    if (s)
	while (*s)
	    push_back((unsigned char)*s++);
}

// init_from_filename() - filenames are supposed to be encoded in
// UTF-8 nowadays, but this is not guaranteed. This method first
// checks if it looks like UTF-8; if not, it assumes it's a
// latin1 (ISO-8859-1) encoding.

void unistring::init_from_filename(const char *filename)
{
    const char *guess = guess_encoding(filename, strlen(filename));
    if (guess && STREQ(guess, "UTF-8"))
	init_from_utf8(filename);
    else
	init_from_latin1(filename);
}

int unistring::index(unichar ch) const
{
    int idx = std::find(begin(), end(), ch) - begin();
    if (idx == len())
	idx = -1;
    return idx;
}

bool unistring::has_char(unichar ch) const
{
    return index(ch) != -1;
}

int unistring::index(const unistring &sub, int from) const
{
    if (from >= len())
	return -1;
    const unichar *pos = std::search(begin() + from, end(),
				     sub.begin(), sub.end());
    if (pos != end())
	return pos - begin();
    else    
	return -1;
}

// locale-independent toupper()
unistring unistring::toupper_ascii() const
{
    unistring ret = *this;
    for (size_type i = 0; i < size(); i++) {
	if (ret[i] >= 'a' && ret[i] <= 'z')
	    ret[i] += 'A' - 'a';
    }
    return ret;
}

void u8string::init_from_unichars(const unichar *src, int len)
{
    char *buf = new char[len * 6 + 1]; // max utf-8 sequence is 6 bytes.
    buf[ unicode_to_utf8(buf, src, len) ] = 0;
    *this = buf;
    delete buf;
}

void u8string::init_from_unichars(const unistring &str)
{
    init_from_unichars(str.begin(), str.size());
}

int u8string::index(const char *s, int from) const
{
    if (from >= len())
	return -1;

    const char *pos = std::search(&*(begin() + from), &*end(),
				     s, s + strlen(s));
    if (pos != &*end())
	return pos - &*begin();
    else
	return -1;
}

inline bool is_ascii_ws(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\n';
}

void u8string::inplace_trim()
{
    while (size() && is_ascii_ws((*this)[0]))
	erase(begin(), begin()+1);
    while (size() && is_ascii_ws((*this)[this->size()-1]))
	erase(end()-1, end());
}

u8string u8string::trim() const
{
    u8string ret = *this;
    ret.inplace_trim();
    return ret;
}

// locale-independent toupper()
u8string u8string::toupper_ascii() const
{
    u8string ret = *this;
    for (size_type i = 0; i < size(); i++) {
	if (ret[i] >= 'a' && ret[i] <= 'z')
	    ret[i] += 'A' - 'a';
    }
    return ret;
}

u8string u8string::erase_char(char xch) const
{
    u8string ret;
    for (size_type i = 0; i < size(); i++) {
	if ((*this)[i] != xch)
	    ret += (*this)[i];
    }
    return ret;
}

void u8string::cformat(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vcformat(fmt, ap);
    va_end(ap);
}

void u8string::vcformat(const char *fmt, va_list ap)
{
#ifdef HAVE_VASPRINTF
    char *buf;
    int result = vasprintf(&buf, fmt, ap);
    if (result != -1 && buf) {
	*this = buf;
	free(buf);
    } else {
	clear();
    }
#else
# define MAX_MSG_LEN 4096
    char buf[MAX_MSG_LEN+1];
    buf[MAX_MSG_LEN] = 0;
# ifdef HAVE_VSNPRINTF
    vsnprintf(buf, MAX_MSG_LEN, fmt, ap);
# else
    vsprintf(buf, fmt, ap);
# endif
    *this = buf;
# undef MAX_MSG_LEN
#endif
}

