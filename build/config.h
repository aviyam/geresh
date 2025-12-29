#ifndef BDE_CONFIG_H
#define BDE_CONFIG_H

#define PACKAGE "geresh"
#define VERSION "0.6.3"

// enable debugging messages?
/* #undef DEBUG */

/* #undef HAVE_NCURSESW_NCURSES_H */
/* #undef HAVE_NCURSES_H */

// are we using wide-curses?
#define HAVE_WIDE_CURSES 1

// do we have nl_langinfo(), CODESET?
/* #undef HAVE_LANGINFO_CODESET */

#define HAVE_LOCALE_H 1
#define HAVE_SETLOCALE 1

// use GNU's gettext?
#define USE_GETTEXT 1
#define LOCALEDIR ""

// does our curses support colors?
#define HAVE_COLOR 1

// do we have the use_default_colors() non-standard function?
/* #undef HAVE_USE_DEFAULT_COLORS */

// can we change cursor visibility with curs_set()?
/* #undef HAVE_CURS_SET */

// wctob() is for 8-bit locales
#define HAVE_WCTOB 1

// btowc() is for 8-bit locales
#define HAVE_BTOWC 1

// can we parse long command-line arguments?
#define HAVE_GETOPT_LONG 1

// use iconv for charset conversion?
#define USE_ICONV 1

// iconv()'s declaration has "const" for the second argument?
#define ICONV_CONST 

// what is iconv's name of our internal encoding?
#define INTERNAL_ENCODING "UCS-4LE"

// default file encoding
#define DEFAULT_FILE_ENCODING "ISO-8859-8"

// DIR and dirent
#define HAVE_DIRENT_H 1
/* #undef HAVE_SYS_NDIR_H */
#define HAVE_SYS_DIR_H 1
/* #undef HAVE_NDIR_H */

// mode_t might need definition on some weird systems, usually standard
// #undef mode_t 

#define RETSIGTYPE void

#define HAVE_VSNPRINTF 1
#define HAVE_VASPRINTF 1

#endif
