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

#include <stdio.h>
#include <errno.h>

#include "transtbl.h"
#include "io.h" // set_last_error
#include "dbg.h"

// Most of the code below deals with parsing a TranslationTable
// file. Such files consist of lines of the form:
//
//   <character-from> <character-to>
//
// that map character-from to character-to.
//
// <character-xxx> can be in one of three forms:
//
// 1. ' literal-character '
// 2. decimal-number .
// 3. hex-number
//
// Examples:
//
// 'a' 5d0    # maps 'a' to Hebrew letter Alef
// 'a' 1488.  # the same
// 'a' 'b'    # maps 'a' to 'b'
//
// literal-character is UTF-8 encoded.


// parse_next_char() - parses the next <character> token. (this is a
// misnomer, because one might think we mean C's "char".)
//
// If there was no lexical error, returns a pointer to the end of the
// token (so one can continue to parse the next token); else returns 
// NULL.

static char *parse_next_char(char *s, unichar &ch)
{
    while (*s == ' ' || *s == '\t')
	s++;
    if (!*s)
	return NULL;
    if (*s == '\'') {
	s++;
	char *end = strchr(s + 1, '\'');
	if (!end)
	    return NULL;
	unistring us;
	us.init_from_utf8(s, end - s);
	if (us.size() != 1)
	    return false;
	ch = us[0];
	return end + 1;
    } else {
	char *end;
	errno = 0;
	int val = strtol(s, &end, 16);
	if (*end == '.') {
	    *end = ' ';
	    val = strtol(s, &end, 10);
	}
	if (errno || (*end != '\0' && *end != ' ' && *end != '\t'))
	    return NULL;
	ch = (unichar)val;
	return end;
    }
}

// load(filename) - loads--that is, parse--a file. It reads the file line by
// line and for each line calls parse_next_char() to parse the two
// <character> tokens. It then adds the mapping to the map table.

bool TranslationTable::load(const char *filename)
{
#define MAX_LINE_LEN 1024
    charmap.clear();

    FILE *fp = fopen(filename, "r");
    if (!fp) {
	set_last_error(errno);
	return false;
    }
    DBG(1, ("Reading translation table %s\n", filename));

    char line[MAX_LINE_LEN];
    while (fgets(line, MAX_LINE_LEN, fp)) {
	int len = strlen(line);
	if (len && line[len-1] == '\n')
	    line[len-1] = 0;
	if (strchr(line, '#'))  // remove comment
	    *(strchr(line, '#')) = '\0';

	unichar ch1, ch2;
	char *s = line;
	if ((s = parse_next_char(s, ch1)))
	    if ((s = parse_next_char(s, ch2)))
		charmap[ch1] = ch2;
    }
    fclose(fp);

    return true;
#undef MAX_LINE_LEN
}

// translate_char() - matches a character with another, in-place. returns
// false if no match exists.

bool TranslationTable::translate_char(unichar &ch) const
{
    std::map<unichar, unichar>::const_iterator
		    it = charmap.find(ch);
    if (it != charmap.end()) {
	ch = it->second;
	return true;
    } else
	return false;
}

