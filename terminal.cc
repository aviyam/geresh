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

// Baruch says to put editor.h first.
#include "editor.h" // for SIGHUP's emergency_save
#include "terminal.h"
#include "dbg.h"

#include <unistd.h> // _POSIX_VDISABLE
#include <sys/wait.h>   // wait()
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <stdlib.h>   // getenv
#include <sys/time.h> // timeval
#include <string.h>   // strstr
#ifdef HAVE_LANGINFO_CODESET
# include <langinfo.h>
#endif

bool terminal::initialized = false;
bool terminal::is_utf8;
bool terminal::force_iso88598;
bool terminal::is_fixed;
bool terminal::is_color;
bool terminal::use_default_colors;
bool terminal::do_arabic_shaping;
bool terminal::graphical_boxes;

static termios oldterm;

void terminal::finish()
{
    endwin();
    tcsetattr(0, TCSANOW, &oldterm);
    DBG(1, ("Bailing out\n"));
}

static RETSIGTYPE sigint_hndlr(int sig)
{
}

static RETSIGTYPE sigterm_hndlr(int sig)
{
    terminal::finish();
    exit(0);
}

static RETSIGTYPE sighup_hndlr(int sig)
{
    Editor::get_global_instance()->emergency_save();
    DBG(1, ("SIGHUP HANDLER\n"));
    exit(0);
}

static RETSIGTYPE sigchld_hndlr(int sig)
{
    int serrno = errno;
    wait(NULL);
    errno = serrno;
}

// was_ctrl_c_pressed() - is a crude method to check if ^C was pressed
// while in a non-interactive segment, like when receiving data from
// the speller. it uses select() to see if there's any keyboard (stdin)
// input available. If so, it eats it up, and checks whether 0x03 (^C)
// was encountered.

bool terminal::was_ctrl_c_pressed()
{
#define HAVE_SELECT 1
#ifdef HAVE_SELECT
    fd_set rfds;
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    bool ctrl_c_pressed = false;

    while (1) {
	FD_ZERO(&rfds);
	FD_SET(STDIN_FILENO, &rfds);
	if (select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv) <= 0)
	    break; // no kbd input avail.
	char ch;
	read(STDIN_FILENO, &ch, 1);
	if (ch == '\x03' || ch == '\x07')
	    ctrl_c_pressed = true;
    }
    return ctrl_c_pressed;
#else
    return false;
#endif
}

// DISABLE_SIGTSTP() is used by child processes (e.g. the speller)
// to get rid of ncurses' handler. See TODO.
void DISABLE_SIGTSTP()
{
    signal(SIGTSTP, SIG_IGN);
}

void terminal::init()
{
    tcgetattr(0, &oldterm);

#ifdef _POSIX_VDISABLE
    termios term;
    term = oldterm;
    term.c_cc[VINTR] = _POSIX_VDISABLE;
    term.c_cc[VQUIT] = _POSIX_VDISABLE;
    term.c_cc[VSTOP] = _POSIX_VDISABLE;
    term.c_cc[VSTART] = _POSIX_VDISABLE;
    tcsetattr(0, TCSANOW, &term);
#else
    termios term;
    term = oldterm;
    term.c_lflag &= ~ISIG;
    term.c_iflag &= ~(IXON | IXOFF);
    tcsetattr(0, TCSANOW, &term);
#endif

    signal(SIGINT, sigint_hndlr);
    signal(SIGTERM, sigterm_hndlr);
    signal(SIGHUP, sighup_hndlr);
    signal(SIGCHLD, sigchld_hndlr);

    // it's important to ignore SIGPIPE because the editor has the ability
    // to write to a pipe.
    signal(SIGPIPE, SIG_IGN);

    initscr();
    keypad(stdscr, TRUE);
    nonl();
    cbreak();
    noecho();

#ifdef HAVE_COLOR
    if (has_colors()) {
	is_color = true;
	start_color();
#ifdef HAVE_USE_DEFAULT_COLORS
	terminal::use_default_colors = (::use_default_colors() == OK);
#else
	terminal::use_default_colors = false;
#endif
    } else {
	is_color = false;
    }
#else
    is_color = false;
#endif

    terminal::do_arabic_shaping = false;
    terminal::initialized = true;

    if (under_x11())
	terminal::graphical_boxes = true;
    else
	terminal::graphical_boxes = false;
}

bool terminal::is_interactive()
{
    return initialized;
}

// Are we running under X?
bool terminal::under_x11()
{
    return getenv("DISPLAY") && *getenv("DISPLAY");
}

// determine_locale() - find out the currently used locale.

void terminal::determine_locale()
{
    const char *locale;
    (void) (((locale = getenv("LC_ALL")) && *locale) ||
    ((locale = getenv("LC_CTYPE")) && *locale) ||
    ((locale = getenv("LANG"))));

    is_utf8 = false;

#ifdef HAVE_LANGINFO_CODESET
# define lcl(subs) strstr(nl_langinfo(CODESET), subs)
#else
# define lcl(subs) (locale && strstr(locale, subs))
#endif

    if (lcl("UTF-8") || lcl("utf-8") || lcl("UTF8") || lcl("utf8"))
        is_utf8 = true;

    // we assume this is a non-fixed terminal if and only if we are in
    // UTF-8 locale and a DISPLAY environment variable is present (the
    // user can change this with command-line options).
    is_fixed = !(is_utf8 && under_x11());
}

