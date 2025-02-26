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

#include <ctype.h> // toupper

#include "converters.h"
#include "iso88598.h"
#include "utf8.h"
#include "dbg.h"

// guess_encoding() - guesses the encoding of a string.

const char *guess_encoding(const char *buf, int len)
{
#define IS_UTF8		 "UTF-8"
#define IS_UTF16	 "UTF-16"
#define IS_UTF32	 "UTF-32"
#define IS_NOT_UTF8	 NULL
#define UNKNOWN_TYPE	 NULL

    if (len >= 4) {
	// check for BOM (Byte-Order Mark):
	// 
	// UTF-16 big-endian          FE FF
	// UTF-16 little-endian       FF FE
	// UTF-32 big-endian          00 00 FE FF
	// UTF-32 little-endian       FF FE 00 00

	if (buf[0] == (char)0xFE && buf[1] == (char)0xFF)
	    return IS_UTF16; // BE
	if (buf[0] == (char)0xFF && buf[1] == (char)0xFE) {
	    if (buf[2] == 0 && buf[3] == 0)
		return IS_UTF32; // LE
	    else
		return IS_UTF16; // LE
	}
	if (buf[0] == 0 && buf[1] == 0
		&& buf[2] == (char)0xFE && buf[3] == (char)0xFF)
	    return IS_UTF32; // BE
    }

    // No BOM was found. Go through the string and check for
    // UTF-8 sequences: if an illegal sequence was found, it's
    // IS_NOT_UTF8, if all squences are legal, it's IS_UTF8, if
    // it doesn't have characters with high bit set, return
    // UNKNOWN. 

    const char *result = UNKNOWN_TYPE;
    int nbytes = 0;
    for (int i = 0; i < len; i++) {
	if (nbytes) {
	    if ((buf[i] & 0xC0) != 0x80) {
		return IS_NOT_UTF8;
	    }
	    if (! --nbytes)
		result = IS_UTF8;
	} else if (buf[i] & 0x80) {
	    const char &c = buf[i];
	    nbytes = (c & 0xE0) == 0xC0 ? 1 :
		     (c & 0xF0) == 0xE0 ? 2 :
		     (c & 0xF8) == 0xF0 ? 3 :
		     (c & 0xFC) == 0xF8 ? 4 :
		     (c & 0xFE) == 0xFC ? 5 : 0;
	    if (nbytes == 0) {
		return IS_NOT_UTF8;
	    }
	}
    }
    return result;
}

#ifdef USE_ICONV
IconvConverter::IconvConverter()
{
    cd = (iconv_t)-1;
}

IconvConverter::~IconvConverter()
{
    if (cd != (iconv_t)-1)
	iconv_close(cd);
}

int IconvConverter::set_source_encoding(const char *encoding)
{
    cd = iconv_open(INTERNAL_ENCODING, encoding);
    if (cd == (iconv_t)-1) {
	return false;
    }
    return true;
}

int IconvConverter::set_target_encoding(const char *encoding)
{
    cd = iconv_open(encoding, INTERNAL_ENCODING);
    if (cd == (iconv_t)-1) {
	return false;
    }
    return true;
}

int IconvConverter::convert(unichar **dest, char **src, int len)
{
    size_t dest_avail = 99999; // :FIXME:  I've tried several xxx_MAX constants
			       //          but it makes iconv() fail.
    size_t src_bytes_left = len;
    size_t result = iconv(cd, (ICONV_CONST char **)src, &src_bytes_left,
			  (char **)dest, &dest_avail);
    return (result == (size_t)-1) ? (int)-1 : (int)result;
}

int IconvConverter::convert(char **dest, unichar **src, int len)
{
    size_t dest_avail = 99999; // :FIXME:
    size_t src_bytes_left = len * sizeof(unichar);
    size_t result;

    while (1) {
	result = iconv(cd, (ICONV_CONST char **)src, &src_bytes_left,
			  dest, &dest_avail);
	if (ilseq_repr && result == (size_t)-1 && errno == EILSEQ) {
	    // We're asked to represent EILSEQ as "?".
	    // we put "?" in **dest, and advance the
	    // src pointer.
	    (*src)++;
	    src_bytes_left -= sizeof(unichar);
	    (**dest) = '?';
	    (*dest)++;
	    dest_avail--;
	} else {
	    break;
	}
    }
    return (result == (size_t)-1) ? (int)-1 : (int)result;
}
#endif

int ISO88598Converter::convert(unichar **dest, char **src, int len)
{
    int count = 0;
    unichar * &d = *dest;
    char *    &s = *src;
    while (len--) {
	*d++ = iso88598_to_unicode(*s++);
	count++;
    }
    return count;
}

int ISO88598Converter::convert(char **dest, unichar **src, int len)
{
    int count = 0;
    char *    &d = *dest;
    unichar * &s = *src;
    while (len--) {
	int ich = unicode_to_iso88598(*s);
	if (ich == EOF) {
	    if (ilseq_repr) {
		ich = '?';
	    } else {
		errno = EILSEQ;
		return -1;
	    }
	}
	*d++ = (char)ich;
	s++;
	count++;
    }
    return count;
}

int Latin1Converter::convert(unichar **dest, char **src, int len)
{
    int count = len;
    unichar * &d = *dest;
    char *    &s = *src;
    while (len--)
	*d++ = (unsigned char)*s++;
    return count;
}

int Latin1Converter::convert(char **dest, unichar **src, int len)
{
    int count = len;
    char *    &d = *dest;
    unichar * &s = *src;
    while (len--) {
	if (*s > 0xFF) {
	    if (ilseq_repr) {
		*d++ = '?';
		s++;		
	    } else {
	    	errno = EILSEQ;
	    	return -1;
	    }
	} else {
	    *d++ = (char)*s++;
	}
    }
    return count;
}

int UTF8Converter::convert(unichar **dest, char **src, int len)
{
    int count = 0;
    unichar * &d = *dest;
    char *    &s = *src;
    const char *problem;
    count = utf8_to_unicode(d, s, len, &problem);
    if (problem) {
	d += count;
	s = (char *)problem;
	errno = EINVAL;
	return -1;
    } else {
	d += count;
	s += len;
    }
    return count;
}

int UTF8Converter::convert(char **dest, unichar **src, int len)
{
    char *    &d = *dest;
    unichar * &s = *src;
    int nbytes = unicode_to_utf8(d, s, len);
    d += nbytes;
    s += len;
    return len;
}

Converter *ConverterFactory::get_internal_converter(const char *enc)
{
    // canonize the encoding name: remove '-', and upperace.
    u8string encoding = u8string(enc).erase_char('-').toupper_ascii();

    DBG(1, ("looking for internal '%s' converter\n", encoding.c_str()));

    if (encoding == "UTF8")
	return new UTF8Converter();
    if (encoding == "ISO88598" || encoding == "88598")
	return new ISO88598Converter();
    if (encoding == "ISO88591" || encoding == "LATIN1"
	    || encoding == "88591" || encoding == "ASCII"
	    || encoding == "USASCII")
	return new Latin1Converter();

    return NULL;
}

Converter *ConverterFactory::get_converter_from(const char *encoding)
{
#ifdef USE_ICONV
    IconvConverter *iconv = new IconvConverter();
    if (!iconv->set_source_encoding(encoding)) {
	delete iconv;
	return NULL;
    } else {
	return iconv;
    }
#else
    return ConverterFactory::get_internal_converter(encoding);
#endif
}

Converter *ConverterFactory::get_converter_to(const char *encoding)
{
#ifdef USE_ICONV
    IconvConverter *iconv = new IconvConverter();
    if (!iconv->set_target_encoding(encoding)) {
	delete iconv;
	return NULL;
    } else {
	return iconv;
    }
#else
    return ConverterFactory::get_internal_converter(encoding);
#endif
}

