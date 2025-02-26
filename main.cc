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

// Baruch says to put io.h first.
#include "io.h"     // get_cfg_filename
#include "pathnames.h"
#include "terminal.h"
#include "editor.h"
#include "themes.h"
#include "directvect.h"
#include "dbg.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h> // strtol
#include <ctype.h>  // isspace
#include <unistd.h> // getopt
#ifdef HAVE_GETOPT_LONG
# include <getopt.h>
#endif
#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

typedef DirectVector<char *> ArgsList;

static int get_num(const char *optname, const u8string &arg, int min, int max)
{
    char *endptr;
    int value = strtol(arg.c_str(), &endptr, 10);
    if (*endptr != '\0' || value < min || value > max)
	fatal(_("Invalid argument for option `%s'. "
		"Valid argument is a numeric value from %d to %d\n"),
		    optname, min, max);
    return value;
}

static bool get_bool(const char *optname, const u8string &arg)
{
    bool result = false;
    if (arg == "on" || arg == "yes" || arg == "true")
	result = true;
    else if (arg == "off" || arg == "no" || arg == "false")
	result = false;
    else
	fatal(_("Invalid argument for option `%s'. "
		"Valid argument is either `on' or `off'.\n"),
		    optname);
    return result;
}

static void help()
{
    printf(_("Usage: %s [options] [[+LINE] file]\n"), PACKAGE);

    printf(_("\nText display:\n"
	"  -a, --dir-algo WORD          Use the WORD algorithm in determining\n"
	"                               the base direction (aka \"paragraph\n"
	"                               embedding level\") of each paragraph:\n"
	"                               `unicode', `contextual-strong',\n"
	"                               `contextual-rtl', `ltr', `rtl'\n"
	"                               (default: `contextual-rtl')\n"
	"  -D, --bidi BOOL              Enable/disable BiDi\n"
	"  -m, --maqaf WORD             How to display the Hebrew makaf:\n"
	"                               `asis', `ascii', `highlight'\n"
	"                               (default: `highlight')\n"
	"  -P, --points WORD            How to display Hebrew/Arabic points:\n"
	"                               `off', `transliterate', `asis'\n"
	"                               (default: `transliterate')\n"
	"  -M, --show-formatting BOOL   Use ASCII characters to represent BiDi\n"
	"                               formatting codes, paragraph endings and\n"
	"                               tabs (default: on)\n"
	"  -A, --arabic-shaping BOOL    Use contextual presentation forms to\n"
	"                               display arabic\n"
	"  -T, --tabsize COLS           Set tab size to COLS (default: 8)\n"
	"                               (synonyms: --tab-width, --tab-size)\n"
	"  -W, --wrap WORD              Set wrap policy to:\n"
	"                               `off', `atwhitespace', `anywhere'\n"
	"                               (default: `atwhitespace')\n"
	"  -Q, --scrollbar WORD         Whether and where to display a scrollbar:\n"
	"                               `none', `left', `right'\n"
	"                               (default: `none')\n"
	"  -G, --syntax BOOL            Auto-detect syntax (for highlighting)\n"
	"                               (default: on)\n"
	"  -U, --underline BOOL         Highlight *text* and _text_\n"
	"                               (default: on)\n"
	"  -g, --theme FILE             Theme to use. FILE should be just\n"
	"                               the base file name, with no directory\n"
	"                               component.\n"
	"                               (e.g.: --theme green.thm)\n"));

    printf(_("\nEditing:\n"
	"  -i, --auto-indent BOOL       Automatically indent new lines\n"
	"  -j, --auto-justify BOOL      Automatically justify long lines\n"
	"  -J, --justification-column COLS\n"
	"                               Set justification column to COLS\n"
	"                               (default: 72)\n"
	"  -e, --rfc2646-trailing-space BOOL\n"
	"                               Keep a trailing space on each line when\n"
	"                               justifying paragraphs (default: on)\n"
	"  -k, --key-for-key-undo BOOL  Allow user to undo discrete keys\n"
	"                               (default: off)\n"
	"  -u, --undo-size SIZE         Limit undo stack to SIZE kilo-bytes\n"
	"                               (default: 50k)\n"
	"  -q, --smart-typing BOOL      Replace some plain ASCII characters with\n"
	"                               typographical ones.\n"
	    ));

    printf(_("\nFiles:\n"
	"  -f, --default-file-encoding ENC\n"
	"                               Set default file encoding to ENC\n"
	"  -F, --file-encoding ENC      Load the file (specified at the command\n"
	"                               line) using the ENC encoding\n"
	"  -S, --suffix SFX             Set file backup extension. Set to\n"
	"                               empty string if you don't want backup.\n"
	"                               (default: read the SIMPLE_BACKUP_SUFFIX\n"
	"                                         environment variable if exists.\n"
	"                                         If it doesn't, use \"~\")\n"));
    
    printf(_("\nTerminal capabilities:\n"
	"  -b, --bw BOOL                Don't use colors\n"
	"  -B, --big-cursor BOOL        Use a big cursor in the console, if possible\n"
	"  -C, --combining-term BOOL    Assume the terminal is capable of displaying\n"
	"                               combining characters and wide (Asian)\n"
	"                               characters\n"
	"  -H, --iso88598-term BOOL     Assume the terminal uses the ISO-8859-8\n"
	"                               encoding (only when not using wide-curses).\n"
	"  -Y, --graphical-boxes WORD   Box characters to use for the pulldown menu\n"
	"                               ans the scrollbar. Valid options:\n"
	"                               `ascii', `graphical', `auto'\n"
	"                               (default: `auto', which means that if the\n"
	"                                         DISPLAY environment variable is\n"
	"                                         set then use graphical chars)\n"));

    printf(_("\nSpeller:\n"
	"  -Z, --speller-cmd CMD        The speller command\n"
	"                               (default: \"ispell -a\")\n"
	"  -X, --speller-encoding ENC   The encoding with which to\n"
	"                               communicate with the speller.\n"
	"                               (default: ISO-8859-1)\n"
	"  (if hspell is found, and no speller command is specified, Geresh\n"
	"   will use it using the correct configuration automatically.)\n"));
	
    printf(_("\nLog2Vis:\n"
	"  -p, --log2vis                log2vis mode.\n"
	"  -E, --log2vis-options OPTS   The option argument OPTS may include:\n"
	"                               `bdo' - to contain the line between\n"
	"                                       LRO and PDF marks.\n"
	"                               `nopad' - do not pad RTL lines on\n"
	"                                       the left.\n"
 	"                               `engpad' - pad LTR lines on the\n"
	"                                       right.\n"
	"                               `emph[:glyph[:marker]]' - turn on\n"
	"                                       underlining.\n"
	"  -t, --log2vis-output-encoding ENC\n"
	"                               Set output encoding for log2vis\n"
	"                               (default: the encoding in which the\n"
	"                                         file was read.)\n"
	"  -w NUM                       Wrap paragraphs at the NUM'th column.\n"
	"                               use `-w0' to turn wrapping off.\n"
	"                               (default: use the COLUMNS environment\n"
	"                                         variable, or 80)\n"));

    printf(_("\nMiscellaneous:\n"
	"  -x, --extrnal-editor CMD     External editor\n"
	"  -c, --show-cursor-position BOOL\n"
	"                               Constantly display the cursor coordinates\n"
	//"  -n, --show-numbers BOOL      Show line numbers\n"
	"  -v, -R, --read-only          Read-only mode\n"
	"  -s, --scroll-step LINES      Set scroll-step to LINES lines\n"
	"                               (default: 1)\n"
	"  -L, --visual-cursor BOOL     Visual cursor movement\n"
	"                               (default: off)\n"
	"  +LINE                        Go to line LINE\n"
	"  -V, --version                Print version info and editor capabilities\n"
	"  -h, --help                   Print usage information\n"));

    printf(_("\nPlease read the User Manual for more information.\n"));
    exit(0);
}

static void parse_rc_line(ArgsList &arguments, const char *s)
{
    const char *token_start = NULL;
    bool in_quote = false;
    if (!s || *s == '#')
	return;

    do {
	if (*s && !isspace(*s) && !token_start) {
	    // starting a new token
	    token_start = s;
	}
	if (*s == '\'' || *s == '\"')
	    in_quote = !in_quote;
	if (token_start && ((isspace(*s) && !in_quote) || !*s)) {
	    // we've found a token.
	    // allocate memory for it and append it
	    // to the argument list.
	    char *param = new char[s - token_start + 1];
	    arguments.push_back(param);
	    // we don't preserve the quoting characters.
	    for (const char *tp = token_start; tp < s; tp++)
		if (*tp != '\'' && *tp != '\"')
		    *param++ = *tp;
	    *param = '\0';
	    token_start = NULL;
	}
    } while (*s++);
}

static bool parse_rc_file(ArgsList &arguments, const char *filename)
{
#define MAX_LINE_LEN 1024
    FILE *fp;
    if ((fp = fopen(filename, "r")) != NULL) {
	DBG(1, ("Reading rc file %s\n", filename));
	char line[MAX_LINE_LEN];
	while (fgets(line, MAX_LINE_LEN, fp)) {
	    if (strchr(line, '#'))
		*(strchr(line, '#')) = '\0';
	    parse_rc_line(arguments, line);
	}
	fclose(fp);
	return true;
    } else {
	return false;
    }	    
#undef MAX_LINE_LEN
}

int main(int argc, char *argv[])
{
    set_debug_level(getenv("GERESH_DEBUG_LEVEL")
			? atoi(getenv("GERESH_DEBUG_LEVEL")) : 0);
    
    DBG(1, ("\n\n----starting----\n\n"));

#ifdef HAVE_SETLOCALE
    if (!setlocale(LC_ALL, ""))
	fatal(_("setlocale() failed! Please check your environment variables "
		"LC_ALL, LC_CTYPE, LANG, etc.\n"));
#endif

    terminal::determine_locale();

#ifdef USE_GETTEXT
    // We enable gettext() only in the UTF-8 locale.
    // That's because gettext() returns text encoded in the locale,
    // whereas all the functions in this app expect UTF-8 text.
    // I'm not going to "fix" this -- UTF-8 locales are the way to go!
    //
    // UPDATE: I can use bind_textdomain_codeset()
    if (terminal::is_utf8) {
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
    }
#endif

    /*
     * Step 1.
     *
     * Build a new argv, which contains arguments read from the RC file,
     * from the command line and from the environment variable $PACKAGE_ARGS.
     * 
     */

    ArgsList arguments;
    arguments.push_back(PACKAGE); // push argv[0] (our program name)

    // Read arguments from the RC file.
    // first, read the RC file pointed to by $PACKAGE_RC.
    // if that fails, read the user's personal RC file, and
    // if that fails too, read the system wide RC file.

    if (!(getenv(PACKAGE "_RC")
	    && parse_rc_file(arguments, get_cfg_filename(getenv(PACKAGE "_RC")))))
    {
	if (!parse_rc_file(arguments, get_cfg_filename(USER_RC_FILE)))
	    parse_rc_file(arguments, get_cfg_filename(SYSTEM_RC_FILE));
    }

    // append the original command line arguments
    arguments.insert(arguments.end(), argv + 1, argv + argc);

    // and finally append all arguments in $PACKAGE_ARGS
    if (getenv(PACKAGE "_ARGS"))
	parse_rc_line(arguments, getenv(PACKAGE "_ARGS"));

    // the standard requires that argv[argc] == NULL
    arguments.push_back(NULL);

    argv = arguments.begin();
    argc = arguments.size() - 1;

    for (int i = 0; i < argc; i++) {
	DBG(1, ("[cmd: %s]\n", argv[i]));
    }

    /*
     * Step 2.
     *
     * Parse all the arguments using getopt().
     * 
     */

    // The following variables hold the values parsed from the
    // arguments. We initialize them to default values.

    bool    auto_justify_flag = false;
    bool    auto_indent_flag = false;
    bool    enable_bidi = true;
    bool    show_formatting_flag = true;
    bool    show_cursor_position_flag = false;
    bool    show_numbers_flag = false;	    // TODO
    bool    read_only_flag = false;
    int	    undo_size = 50;
    bool    key_for_key_undo_flag = false;
    bool    combining_term_flag = false;
    bool    combining_term_flag_specified = false;
    bool    iso88598_term_flag = false;
    bool    big_cursor_flag = false;
    bool    arabic_shaping_flag = false;
    bool    bw_flag = false;
    int	    tab_width = 8;
    int	    justification_column = 72;
    int	    scroll_step = 1;
    bool    smart_typing_flag = false;
    bool    rfc2646_trailing_space_flag = true;
    bool    do_log2vis = false;
    int	    non_interactive_text_width = 80;
    EditBox::WrapType
	    wrap_type = EditBox::wrpAtWhiteSpace;
    diralgo_t
	    dir_algo = algoContextRTL;
    EditBox::rtl_nsm_display_t rtl_nsm = EditBox::rtlnsmTransliterated;
    EditBox::maqaf_display_t maqaf = EditBox::mqfHighlighted;
    Editor::scrollbar_pos_t scrollbar_pos = Editor::scrlbrNone;
    const char
	    *file_encoding = NULL;
    const char
	    *default_file_encoding = NULL;
    const char
	    *log2vis_output_encoding = NULL;
    const char
	    *log2vis_options = NULL;
    const char
	    *backup_suffix = getenv("SIMPLE_BACKUP_SUFFIX")
				? getenv("SIMPLE_BACKUP_SUFFIX") : "~";
    bool
	    print_version_flag = false;
    const char
	    *speller_cmd      = ""; // "ispell -a";
    const char
	    *speller_encoding = ""; // "ISO-8859-1";
    const char
	    *external_editor  = "";
    const char
	    *theme = NULL;
    bool syntax_auto_detection = true;
    bool underline = true;
    bool visual_cursor_movement = false;

    enum { boxesGraphical, boxesAscii, boxesAuto } graphical_boxes = boxesAuto;

#ifdef HAVE_GETOPT_LONG
    static struct option long_options[] = {
	{ "tab-width",	    1, 0, 'T' },
	{ "tabwidth",	    1, 0, 'T' },
	{ "tab-size",	    1, 0, 'T' },
	{ "tabsize",	    1, 0, 'T' },
	{ "justification-column", 1, 0, 'J' },
	{ "rfc2646-trailing-space", 1, 0, 'e' },
	{ "wrap",	    1, 0, 'W' },
	{ "dir-algo",	    1, 0, 'a' },
	{ "undo-size",	    1, 0, 'u' },
	{ "key-for-key-undo", 1, 0, 'k' },
	{ "auto-justify",   1, 0, 'j' },
	{ "auto-indent",    1, 0, 'i' },
	{ "show-formatting",1, 0, 'M' },
	{ "maqaf",	    1, 0, 'm' },
	{ "points",         1, 0, 'P' },
	{ "show-cursor-position", 1, 0, 'c' },
	{ "show-numbers",   1, 0, 'n' },
	{ "smart-typing",   1, 0, 'q' },
	{ "default-file-encoding", 1, 0, 'f' },
	{ "file-encoding",  1, 0, 'F' },
	{ "combining-term", 1, 0, 'C' },
	{ "iso88598-term",  1, 0, 'H' },
	{ "arabic-shaping", 1, 0, 'A' },
	{ "read-only",	    0, 0, 'R' },
	{ "suffix",	    1, 0, 'S' },
	{ "bw",		    1, 0, 'b' },
	{ "big-cursor",	    1, 0, 'B' },
	{ "version",	    0, 0, 'V' },
	{ "help",	    0, 0, 'h' },
	{ "scroll-step",    1, 0, 's' },
	{ "log2vis",	    0, 0, 'p' },
	{ "log2vis-output-encoding", 1, 0, 't' },
	{ "log2vis-options",  1, 0, 'E' },
	{ "speller-cmd",      1, 0, 'Z' },
	{ "speller-encoding", 1, 0, 'X' },
	{ "graphical-boxes",  1, 0, 'Y' },
	{ "scrollbar",	      1, 0, 'Q' },
	{ "bidi",	      1, 0, 'D' },
	{ "syntax",	      1, 0, 'G' },
	{ "underline",	      1, 0, 'U' },
	{ "visual-cursor",    1, 0, 'L' },
	{ "theme",	      1, 0, 'g' },
	{ "external-editor",  1, 0, 'x' },
	{0, 0, 0, 0}
    };
#endif

    if (getenv("COLUMNS"))
	non_interactive_text_width = atoi(getenv("COLUMNS"));

    const char *short_options = "T:e:J:W:w:a:A:k:S:s:u:j:i:P:M:m:c:n:q:f:F:C:H:RvB:b:Vhpt:E:Z:X:Y:Q:D:G:g:U:L:x:";
    int c;
#ifdef HAVE_GETOPT_LONG
    int long_idx = -1;
    while ((c = getopt_long(argc, argv, short_options,
			    long_options, &long_idx)) != -1) {
#else
    while ((c = getopt(argc, argv, short_options)) != -1) {
#endif

	u8string optname, arg;
	
#ifdef HAVE_GETOPT_LONG
	if (long_idx != -1) {
	    optname = "--";
	    optname += long_options[long_idx].name;
	} else
#endif
	{
	    optname = "-";
	    optname += (char)c;
	}

	arg = optarg ? optarg : "";

#define GET_BOOL()	    get_bool(optname.c_str(), arg)
#define GET_NUM(min, max)   get_num(optname.c_str(), arg, min, max)
	switch (c) {
	case 'j': auto_justify_flag = GET_BOOL(); break;
	case 'i': auto_indent_flag = GET_BOOL(); break;
	case 'M': show_formatting_flag = GET_BOOL(); break;
	case 'c': show_cursor_position_flag = GET_BOOL(); break;
	case 'n': show_numbers_flag = GET_BOOL(); break;
	case 'q': smart_typing_flag = GET_BOOL(); break;
	case 'C': 
		  combining_term_flag_specified = true;
		  combining_term_flag = GET_BOOL();
		  break;
	case 'H': 
#ifdef HAVE_WIDE_CURSES
		  fatal(_("The `%s' option is now valid only when NOT "
			  "compiling Geresh against wide-curses. (Why do "
			  "you want to use it?)\n"),
			  optname.c_str());
#endif
		  iso88598_term_flag = GET_BOOL();
		  break;
	case 'b': bw_flag = GET_BOOL(); break;
	case 'A': arabic_shaping_flag = GET_BOOL(); break;
	case 'B': big_cursor_flag = GET_BOOL(); break;
	case 'k': key_for_key_undo_flag = GET_BOOL(); break;
	case 'R':
	case 'v':
		  read_only_flag = true;
		  break;
	case 'T': tab_width = GET_NUM(1, 80); break;
	case 'J': justification_column = GET_NUM(1, 999999); break;
	case 'e': rfc2646_trailing_space_flag = GET_BOOL(); break;
	case 'u': undo_size = GET_NUM(0, 999); break;
	case 's': scroll_step = GET_NUM(1, 999); break;
	case 'D': enable_bidi = GET_BOOL(); break;
	case 'G': syntax_auto_detection = GET_BOOL(); break;
	case 'U': underline = GET_BOOL(); break;
	case 'L': visual_cursor_movement = GET_BOOL(); break;
	
	case 'W':
		if (arg == "off")
		    wrap_type = EditBox::wrpOff;
		else if (arg == "atwhitespace")
		    wrap_type = EditBox::wrpAtWhiteSpace;
		else if (arg == "anywhere")
		    wrap_type = EditBox::wrpAnywhere;
		else
		    fatal(_("invalid argument `%s' for `%s'\n"
			    "Valid arguments are:\n"
			    "%s"),
			    optarg, optname.c_str(),
			    _("  - `off'\n"
			      "  - `atwhitespace'\n"
			      "  - `anywhere'\n"));
		break;
	case 'a':
		if (arg == "unicode")
		    dir_algo = algoUnicode;
		else if (arg == "contextual-strong")
		    dir_algo = algoContextStrong;
		else if (arg == "contextual-rtl")
		    dir_algo = algoContextRTL;
		else if (arg == "ltr")
		    dir_algo = algoForceLTR;
		else if (arg == "rtl")
		    dir_algo = algoForceRTL;
		else
		    fatal(_("invalid argument `%s' for `%s'\n"
			    "Valid arguments are:\n"
			    "%s"),
			    optarg, optname.c_str(),
			    _("  - `unicode'\n"
			      "  - `contextual-strong'\n"
			      "  - `contextual-rtl'\n"
			      "  - `ltr'\n"
			      "  - `rtl'\n"));
		break;
	case 'm':
		if (arg == "asis")
		    maqaf = EditBox::mqfAsis;
		else if (arg == "ascii")
		    maqaf = EditBox::mqfTransliterated;
		else if (arg == "highlight")
		    maqaf = EditBox::mqfHighlighted;
		else
		    fatal(_("invalid argument `%s' for `%s'\n"
			    "Valid arguments are:\n"
			    "%s"),
			    optarg, optname.c_str(),
			    _("  - `asis'\n"
			      "  - `ascii'\n"
			      "  - `highlight'\n"));
		break;
	case 'P':
		if (arg == "off")
		    rtl_nsm = EditBox::rtlnsmOff;
		else if (arg == "transliterate")
		    rtl_nsm = EditBox::rtlnsmTransliterated;
		else if (arg == "asis")
		    rtl_nsm = EditBox::rtlnsmAsis;
		else
		    fatal(_("invalid argument `%s' for `%s'\n"
			    "Valid arguments are:\n"
			    "%s"),
			    optarg, optname.c_str(),
			    _("  - `off'\n"
			      "  - `transliterate'\n"
			      "  - `asis'\n"));
		break;
	case 'Y':
		if (arg == "graphical")
		    graphical_boxes = boxesGraphical;
		else if (arg == "ascii")
		    graphical_boxes = boxesAscii;
		else if (arg == "auto")
		    graphical_boxes = boxesAuto;
		else
		    fatal(_("invalid argument `%s' for `%s'\n"
			    "Valid arguments are:\n"
			    "%s"),
			    optarg, optname.c_str(),
			    _("  - `graphical'\n"
			      "  - `ascii'\n"
			      "  - `auto'\n"));
		break;
	case 'Q':
		if (arg == "none")
		    scrollbar_pos = Editor::scrlbrNone;
		else if (arg == "left")
		    scrollbar_pos = Editor::scrlbrLeft;
		else if (arg == "right")
		    scrollbar_pos = Editor::scrlbrRight;
		else
		    fatal(_("invalid argument `%s' for `%s'\n"
			    "Valid arguments are:\n"
			    "%s"),
			    optarg, optname.c_str(),
			    _("  - `none'\n"
			      "  - `left'\n"
			      "  - `right'\n"));
		break;
	case 'F': file_encoding = optarg; break;
	case 'f': default_file_encoding = optarg; break;
	case 't': log2vis_output_encoding = optarg; break;
	case 'S': backup_suffix = optarg; break;
	case 'p':
		do_log2vis	     = true;
		arabic_shaping_flag  = true;
		show_formatting_flag = false;
		rtl_nsm              = EditBox::rtlnsmAsis;
		break;
	case 'E': log2vis_options = optarg; break;
	case 'w': non_interactive_text_width = GET_NUM(0, 9999); break;	
	case 'Z': speller_cmd = optarg; break;
	case 'X': speller_encoding = optarg; break;
	case 'g': theme = optarg; break;
	case 'x': external_editor = optarg; break;
	case 'V': print_version_flag = true; break;
	case 'h': help(); break;
	case '?':
	    fatal(NULL);
	    break;
	}
#ifdef HAVE_GETOPT_LONG
	long_idx = -1;
#endif
    }
#undef GET_BOOL
#undef GET_NUM

    if (non_interactive_text_width == 0)
	wrap_type = EditBox::wrpOff;

    /*
     * Step 3.
     *
     * Initialize the terminal.
     * 
     */

    if (iso88598_term_flag) {
	terminal::is_utf8 = false;
	terminal::force_iso88598 = true;
	terminal::is_fixed = true;
    }
    if (combining_term_flag_specified) {
	terminal::is_fixed = !combining_term_flag;
    }

#ifndef HAVE_WIDE_CURSES
    if (terminal::is_utf8)
	fatal(_("Terminal locale is UTF-8, but %s was not compiled with a "
		"curses library that supports wide characters (ncursesw). "
		"Please read the INSTALL document that accompanies Geresh "
		"to get more information.\n"), PACKAGE);
#endif

    // the -V option ("print version") is very useful when we want to
    // find out what the program thinks our terminal capabilities are.

    if (print_version_flag) {
	printf(_("%s version %s\n"), PACKAGE, VERSION);
#ifdef HAVE_WIDE_CURSES
	printf(_("Compiled with wide-curses.\n"));
#endif
	printf(_("\nTerminal encoding:\n%s, %s\n"),
		    terminal::is_utf8
			    ? "UTF-8"
			    : "8-bit",
		    terminal::is_fixed
			    ? _("cannot handle combining/wide characters")
			    : _("can handle combining/wide characters"));
	if (terminal::force_iso88598)
	    printf(_("I'll treat this terminal as ISO-8859-8\n"));
	printf(_("\nFiles I can use:\n"));
	const char * files[] = {
	    USER_RC_FILE,
	    USER_TRANSTBL_FILE,
	    USER_REPRTBL_FILE,
	    USER_ALTKBDTBL_FILE,
	    USER_MANUAL_FILE,
	    USER_UNICODE_DATA_FILE,
	    USER_THEMES_DIR,
	    SYSTEM_RC_FILE,
	    SYSTEM_TRANSTBL_FILE,
	    SYSTEM_REPRTBL_FILE,
	    SYSTEM_ALTKBDTBL_FILE,
	    SYSTEM_MANUAL_FILE,
	    SYSTEM_UNICODE_DATA_FILE,
	    SYSTEM_THEMES_DIR,
	    NULL
	};
	int i = 0;
	while (files[i])
	    printf("%s\n", get_cfg_filename(files[i++]));
	exit(0);
    }

    if (!do_log2vis) {
	terminal::init();
	if (bw_flag)
	    terminal::is_color = false;
	if (graphical_boxes != boxesAuto)
	    terminal::graphical_boxes = (graphical_boxes == boxesGraphical);
	
	ThemeError theme_error;
	bool rslt;
	if (!theme || !terminal::is_color)
	    rslt = load_default_theme(theme_error);
	else
	    rslt = load_theme(theme, theme_error);
	if (!rslt) {
	    terminal::finish();
	    fatal(_("Can't load theme: %s\n"), theme_error.format().c_str());
	}
    } else {
	terminal::is_fixed = false;
	terminal::is_utf8  = true;
    }

    /*
     * Step 3.
     *
     * Instantiate and initialize an Editor object.
     * Load the file specified on the command line.
     *
     */

    Editor bde;

    bde.set_tab_width(tab_width);
    bde.set_justification_column(justification_column);
    bde.set_rfc2646_trailing_space(rfc2646_trailing_space_flag);
    bde.set_wrap_type(wrap_type);
    bde.set_dir_algo(dir_algo);
    bde.set_scroll_step(scroll_step);
    bde.set_maqaf_display(maqaf);
    bde.set_rtl_nsm_display(rtl_nsm);
    bde.set_backup_suffix(backup_suffix);
    bde.set_smart_typing(smart_typing_flag);
    bde.set_undo_size_limit(undo_size * 1024);
    bde.set_key_for_key_undo(key_for_key_undo_flag);
    bde.set_read_only(read_only_flag);
    bde.set_speller_cmd(speller_cmd);
    bde.set_speller_encoding(speller_encoding);
    bde.adjust_speller_cmd();
    bde.set_external_editor(external_editor);
    bde.set_scrollbar_pos(scrollbar_pos);
    bde.enable_bidi(enable_bidi);
    bde.set_syntax_auto_detection(syntax_auto_detection);
    bde.set_underline(underline);
    bde.set_visual_cursor_movement(visual_cursor_movement);
    if (default_file_encoding)
	bde.set_default_encoding(default_file_encoding);
    if (arabic_shaping_flag)
	bde.toggle_arabic_shaping();
    if (auto_indent_flag)
	bde.toggle_auto_indent();
    if (auto_justify_flag)
	bde.toggle_auto_justify();
    if (!show_formatting_flag)
	bde.toggle_formatting_marks();
    if (show_cursor_position_flag)
	bde.toggle_cursor_position_report();
#ifdef HAVE_CURS_SET
    if (!do_log2vis && big_cursor_flag)
	bde.set_big_cursor(true);
#endif

    // Load file

    const char *filename = NULL;
    int go_to_line = 0;
    for (int i = optind; i < argc; i++) {
	if (*argv[i] == '+')
	    go_to_line = atol(argv[i]);
	else
	    filename = argv[i];
    }

    if (do_log2vis) {
	bde.set_non_interactive_text_width(non_interactive_text_width);
	if (filename) {
	    bde.load_file(filename, file_encoding);
	    if (bde.is_new()) {
		// a crude method to print an error message
		// when the file does not exist.
		bde.insert_file(filename, file_encoding);
	    }
	} else {
	    bde.load_file("-", file_encoding);
	}
	if (log2vis_output_encoding)
	    bde.set_encoding(log2vis_output_encoding);
    } else {
	if (filename)
	    bde.load_file(filename, file_encoding);
	if (go_to_line > 0)
	    bde.go_to_line(go_to_line);
    }
    
    /*
     * Step 4.
     *
     * Start the event pump.
     * 
     */

    if (!do_log2vis) {
	bde.exec();
    } else {
	bde.log2vis(log2vis_options);
	bde.save_file("-", bde.get_encoding());
    }

    if (!do_log2vis) {
	terminal::finish();
    }

    return 0;
}

