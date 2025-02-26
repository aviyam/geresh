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

#ifndef BDE_CONVERTERS_H
#define BDE_CONVERTERS_H

#include <config.h>

#include <errno.h>
#ifndef EILSEQ
# define EILSEQ 2000
#endif

#ifdef USE_ICONV
# include <iconv.h>
#endif

#include "types.h"

// A Converter object converts between encodings.
//
//  Converter (an abstract class)
//  |
//  +-- IconvConverter	    (used when the system has ICONV)
//  |
//  +-- ISO88598Converter   (used when no ICONV presents)
//  +-- Latin1Converter	    ditto
//  +-- UTF8Converter	    ditto

const char *guess_encoding(const char *buf, int len);

class Converter {
protected:
    bool ilseq_repr;
public:
    // The Converter interface is based on iconv()'s interface.
    // if a conversion function fails, it returns -1 and updates errno.
    virtual int convert(unichar **dest, char **src, int len) = 0;
    virtual int convert(char **dest, unichar **src, int len) = 0;
    Converter() {
	ilseq_repr = false;
    }
    virtual ~Converter() { }
    void enable_ilseq_repr(bool val = true) {
	ilseq_repr = val;
    }
};

#ifdef USE_ICONV
class IconvConverter : public Converter {
    iconv_t cd;
public:
    IconvConverter();
    virtual ~IconvConverter();

    int set_source_encoding(const char *encoding);
    int set_target_encoding(const char *encoding);
    virtual int convert(unichar **dest, char **src, int len);
    virtual int convert(char **dest, unichar **src, int len);
};
#endif

class ISO88598Converter : public Converter {
public:
    virtual int convert(unichar **dest, char **src, int len);
    virtual int convert(char **dest, unichar **src, int len);
};

class Latin1Converter : public Converter {
public:
    virtual int convert(unichar **dest, char **src, int len);
    virtual int convert(char **dest, unichar **src, int len);
};

class UTF8Converter : public Converter {
public:
    virtual int convert(unichar **dest, char **src, int len);
    virtual int convert(char **dest, unichar **src, int len);
};

class ConverterFactory {
    static Converter *get_internal_converter(const char *encoding);
public:
    static Converter *get_converter_from(const char *encoding);
    static Converter *get_converter_to(const char *encoding);
};

#endif

