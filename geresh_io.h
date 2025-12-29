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

#ifndef BDE_IO_H
#define BDE_IO_H

#include "types.h"

class EditBox;

bool xload_file(EditBox *editbox,
		const char *filename,
		const char *specified_encoding, 
		const char *default_encoding,
		u8string &effective_encoding,
		bool &is_new,
		bool new_document);

bool xsave_file(EditBox *editbox,
		const char *filename,
		const char *specified_encoding,
		const char *backup_suffix,
		unichar &offending_char,
		bool selection_only = false);

void set_last_error(const char *fmt, ...);
void set_last_error(int err);
const char *get_last_error();

bool has_prog(const char *progname);
void expand_tilde(u8string &filename);
const char *get_cfg_filename(const char *tmplt);

#endif

