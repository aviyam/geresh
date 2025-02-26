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
#include <fcntl.h>  // file primitives
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h> // strerror
#include <pwd.h>    // getpwuid

#include <map>

#include "io.h"
#include "editbox.h"
#include "converters.h"
#include "dbg.h"
#include "speller.h" // for UNLOAD_SPELLER(), see TODO

#define CONVBUFSIZ 8192

static u8string err_msg; 

void set_last_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_msg.vcformat(fmt, ap);
    va_end(ap);
}

void set_last_error(int err) {
    err_msg = strerror(err);
}

const char *get_last_error() {
    return err_msg.c_str();
}

static bool xload_file(EditBox *editbox,
		       int fd,
		       const char *specified_encoding, 
		       const char *default_encoding,
		       u8string &effective_encoding)
{
    unichar outbuf[CONVBUFSIZ+1];
    char inbuf[CONVBUFSIZ];
    bool result = true;
    const char *encoding = NULL; // silence the compiler
    Converter *conv = NULL;

    // we set "effective_encoding" here too, because the
    // file might be empty and the next assignment won't be
    // executed at all.
    if (specified_encoding && *specified_encoding)
	effective_encoding = specified_encoding;
    else
	effective_encoding = default_encoding;

    size_t insize = 0;
    size_t buf_file_offset = 0;
    while (1) {
	ssize_t nread;
	char *inptr = inbuf;
 
	nread = read(fd, inbuf + insize, sizeof(inbuf) - insize);
	if (nread == 0) {
	    // no more input
	    break;
	}
	if (nread == -1) {
	    set_last_error(errno);
	    result = false;
	    break;
	}
	insize += nread;

	// instantiate a Converter object
	if (!conv) {
	    if (!specified_encoding || !*specified_encoding) {
		const char *guess = guess_encoding(inbuf, insize);
		if (guess)
		    encoding = guess;
		else
		    encoding = default_encoding;
	    } else {
		encoding = specified_encoding;
	    }
	    conv = ConverterFactory::get_converter_from(encoding);
	    if (!conv) {
		set_last_error(_("Conversion from '%s' not available"),
				    encoding);
		result = false;
		break;
	    }
	    effective_encoding = encoding;
	}

	unichar *wrptr = outbuf;
	// do the conversion
	int nconv = conv->convert(&wrptr, &inptr, insize);

	// load into editbox
	editbox->transfer_data(outbuf, wrptr - outbuf);

	if (nconv == -1) {
	    insize = inbuf + insize - inptr;
	    if (errno == EINVAL) {
		// incomplete byte sequence. move the unused bytes
		// to the beginning of the buffer. the next read()
		// will complete the sequence.
		memmove(inbuf, inptr, insize);
	    } else {
		// invalid byte sequence.
		if (errno == EILSEQ)
		    set_last_error(_("'%s' conversion failed at position %d"),
			    encoding, buf_file_offset + (inptr - inbuf));
		else
		    set_last_error(_("'%s' conversion failed"), encoding);
		result = false;
		break;
	    }
	} else {
	    insize = 0;
	}
	buf_file_offset += nread;
    }

    if (conv)
	delete conv;
 
    return result;
}

// xload_file() - loads a file into an EditBox buffer.

bool xload_file(EditBox *editbox,
		const char *filename,
		const char *specified_encoding,
		const char *default_encoding,
		u8string &effective_encoding,
		bool &is_new,
		bool new_document)
{
    int  fd;
    bool is_pipe = false;
    FILE *pipe_stream = NULL;
    bool is_stdin = false;
    
    if (filename[0] == '-' && filename[1] == '\0')
	is_stdin = true;

    if (filename[0] == '|' || filename[0] == '!') {
	filename++;
	// we use UTF-8 for pipe communication
	specified_encoding = "UTF-8";
	is_pipe = true;
    }
    
    editbox->start_data_transfer(EditBox::dataTransferIn, new_document);
    
    if (is_pipe) {
	UNLOAD_SPELLER(); // see TODO
	is_new = true;
	pipe_stream = popen(filename, "r");
	if (pipe_stream == NULL) {
	    editbox->end_data_transfer();
	    set_last_error(errno);
	    return false;
	}
	fd = fileno(pipe_stream);
    } else {
	is_new = false;
	if (is_stdin)
	    fd = STDIN_FILENO;
	else
	    fd = open(filename, O_RDONLY);
	if (fd == -1) {
	    editbox->end_data_transfer();
	    if (errno == ENOENT && new_document) {
		is_new = true;
		return true;
	    } else {
		set_last_error(errno);
		return false;
	    }
	}
    }

    bool result = xload_file(editbox, fd, specified_encoding,
			     default_encoding, effective_encoding);
    editbox->end_data_transfer();

    if (is_pipe)
	pclose(pipe_stream);
    else {
	if (!is_stdin)
	    close(fd);
    }
    return result;
}

static bool xsave_file(EditBox *editbox,
		       int fd,
		       const char *encoding,
		       unichar &offending_char)
{
    char outbuf[CONVBUFSIZ*6];
    unichar inbuf[CONVBUFSIZ];
    bool result = true;
    Converter *conv = NULL;
    
    conv = ConverterFactory::get_converter_to(encoding);
    
    if (!conv) {
	set_last_error(_("Conversion to '%s' not available"), encoding);
	return false;
    }

    int edit_buf_offset = 0;
    while (1) {
	int nread = editbox->transfer_data(inbuf, sizeof(inbuf)/sizeof(unichar));
	if (nread == 0) {
	    // We reached end of buffer.
	    // :TODO: zero output state.
	    break;
	}
	
	char    *wrptr = outbuf;
	unichar *inptr = inbuf;
	// do the conversion
	int nconv = conv->convert(&wrptr, &inptr, nread);

	if (write(fd, outbuf, wrptr - outbuf) == -1) {
	    set_last_error(errno);
	    result = false;
	    break;
	}
	
	if (nconv == -1) {
	    // Probably some unicode character couldn't be converted
	    // to the requested encoding.
	    if (errno == EILSEQ) {
		int illegal_pos = inptr - inbuf;
		offending_char = inbuf[illegal_pos];
		set_last_error(_("'%s' conversion failed at position "
				    "%d (char: U+%04X)"),
				encoding, edit_buf_offset + illegal_pos,
				offending_char);
	    } else {
		set_last_error(_("'%s' conversion failed"), encoding);
	    }
	    result = false;
	    break;
	}
	edit_buf_offset += nread;
    }

    if (conv)
	delete conv;
 
    return result;
}

// xsave_file() - saves an EditBox buffer to a file.

bool xsave_file(EditBox *editbox,
		const char *filename,
		const char *specified_encoding,
		const char *backup_suffix,
		unichar &offending_char,
		bool selection_only)
{
    offending_char = 0;

    int  fd;
    bool is_pipe = false;
    FILE *pipe_stream = NULL;
    u8string tmp_filename;
    bool is_stdout = false;
    
    if (filename[0] == '-' && filename[1] == '\0')
	is_stdout = true;

    if (filename[0] == '|' || filename[0] == '!') {
	filename++;
	// we use UTF-8 for pipe communication
	specified_encoding = "UTF-8";
	is_pipe = true;
    }

    if (is_pipe) {
	UNLOAD_SPELLER(); // see TODO
	pipe_stream = popen(filename, "w");
	if (pipe_stream == NULL) {
	    set_last_error(errno);
	    return false;
	}
	fd = fileno(pipe_stream);
    } else if (is_stdout) {
	fd = STDOUT_FILENO;
    } else {
	// 1. Get filename's permissions, if exists.
	struct stat file_info;
	mode_t permissions;
	if (stat(filename, &file_info) != -1)
	    permissions = (file_info.st_mode & 0777);
	else
	    permissions = 0666;
	
	// 2. Create filename.tmp and write data into it.
	//    Use filename's permissions.
	tmp_filename = filename;
	tmp_filename += ".tmp";
	fd = open(tmp_filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
		      permissions);
	if (fd == -1) {
	    set_last_error(errno);
	    return false;
	}
    }

    editbox->start_data_transfer(EditBox::dataTransferOut, false, selection_only);
    bool result = xsave_file(editbox, fd, specified_encoding, offending_char);
    editbox->end_data_transfer();

    if (is_pipe) {
	pclose(pipe_stream);
    } else if (is_stdout) {
	if (!result) {
	    return false;
	}
    } else {
	if (!result) {
	    close(fd);
	    unlink(tmp_filename.c_str());
	    return false;
	}
	if (close(fd) == -1) {
	    set_last_error(errno);
	    unlink(tmp_filename.c_str());
	    return false;
	}

	// 3. if the user wants a backup,
	//    mv filename -> filename${backup_extension}
	//    We ignore errors, since filename may not exist.
	if (backup_suffix && *backup_suffix) {
	    u8string backup = filename;
	    backup += backup_suffix;
	    rename(filename, backup.c_str());
	}
	// Else, unlink filename
	else {
	    unlink(filename);
	}
      
	// 4. rename filename.tmp to filename
	if (rename(tmp_filename.c_str(), filename) == -1) {
	    set_last_error(errno);
	    return false;
	}
    }
    
    return true;
}

// has_prog() returns true if progname is in the PATH and is executable.

bool has_prog(const char *progname)
{
    const char *path = getenv("PATH");
    if (!path) return false;
    const char *pos = path;
    while (pos) {
	u8string comp;
	pos = strchr(path, ':');
	if (!pos)
	    comp = u8string(path);
	else
	    comp = u8string(path, pos);
	comp += u8string("/") + progname;
	if (access(comp.c_str(), X_OK) == 0) {
	    //cerr << "found [" << comp.c_str() << "]\n";
	    return true;
	}
	path = pos + 1;
    }
    return false;
}

// expand_tilde() - do tilde expansion in filenames.

void expand_tilde(u8string &filename)
{
    int slash_pos = filename.index('/');

    if (slash_pos == -1 || filename[0] != '~')
	return; // nothing to do

    u8string tilde_prefix(&*(filename.begin() + 1),
			  &*(filename.begin() + slash_pos));
    u8string user_home;

    if (tilde_prefix == "") {
	// handle "~/" - the user executing us.
	if (getenv("HOME"))
	    user_home = getenv("HOME");
	else 
	    user_home = getpwuid(geteuid())->pw_dir;
    } else {
	// handle "~username/"
	struct passwd *user_info;
	if ((user_info = getpwnam(tilde_prefix.c_str()))) {
	    user_home = user_info->pw_dir;
	} 
    }

    // if we've found the user's home, replace tilde_prefix with it.
    if (!user_home.empty()) {
	if (*(user_home.end() - 1) != '/')
	    user_home += "/";
	filename.replace(0, tilde_prefix.size() + 2, user_home);
    }
}

struct ltstr {
    bool operator() (const char *s1, const char *s2) const {
	return strcmp(s1, s2) < 0;
    }
};

// get_cfg_filename() - does "%P" and tilde expansion on a package
// pathname. in other words, converts a package pathname (~/%P/name)
// to a filesystem pathname (/home/david/geresh/name).

const char *get_cfg_filename(const char *tmplt)
{
    // save the expansion result in a hash
    static std::map<const char *, u8string, ltstr> filenames_cache;

    if (filenames_cache.find(tmplt) == filenames_cache.end()) {
	u8string filename = tmplt;
	// replace all "%P" with PACKAGE
	const char *packpos;    
	while ((packpos = strstr(filename.c_str(), "%P")) != NULL)
	    filename.replace(packpos - filename.c_str(), 2, PACKAGE);

	expand_tilde(filename);
	filenames_cache[tmplt] = filename;
    }
    return filenames_cache[tmplt].c_str();
}

