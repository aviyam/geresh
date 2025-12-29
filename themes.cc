#include <config.h>

#include "geresh_io.h"     // get_cfg_filename
#include "pathnames.h"
#include "widget.h"
#include "themes.h"

#include <errno.h>

#include <map>

#ifdef HAVE_COLOR

#define MISSING_COLOR	-600
#define NO_COLOR	-601
#define SKIP_COLOR	-602

// An ATTR structure describes the attributes (color, etc) of
// a single GUI element.

struct ATTR {
    int  ident;
    char *name; // the name of the GUI element in the theme file

    // These two members point to a parent ATTR struct
    // from which the foreground & background colors will
    // be read if they are missing in the theme file.
    int fg_parent;
    int bg_parent;

    int fg;	// holds color number
    int bg;	// "
    int extra;	// A_BOLD, A_UNDERLINE, etc...
    int pair;	// init_pair(fg, bg)

    void clear()
    {
	// clear only the properties that are read from disk,
	// not those initialized in this file (because the
	// latter are unchangeable).
	fg = bg = MISSING_COLOR;
	extra = 0;
	pair  = 0;
    }
};

ATTR attr_table[] = {
    { MENUBAR_ATTR, "menubar", MENU_ATTR, MENU_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { MENU_ATTR, "menu", MENU_ATTR, MENU_ATTR,
	-1, -1, 0, 0 },
    { MENU_SELECTED_ATTR, "menu.selected", MENU_ATTR, MENU_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { MENU_FRAME_ATTR, "menu.frame", MENU_ATTR, MENU_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { MENU_LETTER_ATTR, "menu.letter", MENU_ATTR, MENU_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { MENU_LETTER_SELECTED_ATTR, "menu.letter.selected", MENU_LETTER_ATTR, MENU_SELECTED_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { MENU_INDICATOR_ATTR, "menu.indicator", MENU_ATTR, MENU_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { MENU_INDICATOR_SELECTED_ATTR, "menu.indicator.selected", MENU_INDICATOR_ATTR, MENU_SELECTED_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { STATUSLINE_ATTR, "statusline",  MENUBAR_ATTR, MENUBAR_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_ATTR, "edit",  EDIT_ATTR, EDIT_ATTR,
	-1, -1, 0, 0 },
    { DIALOGLINE_ATTR, "dialogline", EDIT_ATTR, EDIT_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { SCROLLBAR_ATTR, "scrollbar", EDIT_ATTR, EDIT_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { SCROLLBAR_THUMB_ATTR, "scrollbar.thumb", MENUBAR_ATTR, MENUBAR_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_FAILED_CONV_ATTR, "edit.failed-conversion", EDIT_ATTR, EDIT_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_CONTROL_ATTR, "edit.control", EDIT_ATTR, EDIT_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_EOP_ATTR, "edit.eop", EDIT_ATTR, EDIT_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_EXPLICIT_ATTR, "edit.explicit-bidi", EDIT_EOP_ATTR, EDIT_EOP_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_NSM_ATTR, "edit.nsm", EDIT_EXPLICIT_ATTR, EDIT_EXPLICIT_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_NSM_HEBREW_ATTR, "edit.nsm.hebrew", EDIT_NSM_ATTR, EDIT_NSM_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_NSM_CANTILLATION_ATTR, "edit.nsm.cantillation", EDIT_NSM_HEBREW_ATTR, EDIT_NSM_HEBREW_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_NSM_ARABIC_ATTR, "edit.nsm.arabic", EDIT_NSM_ATTR, EDIT_NSM_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_TAB_ATTR, "edit.tab", EDIT_EOP_ATTR, EDIT_EOP_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_WIDE_ATTR, "edit.wide", EDIT_NSM_ATTR, EDIT_NSM_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_TRIM_ATTR, "edit.trim", EDIT_ATTR, EDIT_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_WRAP_ATTR, "edit.wrap", EDIT_TRIM_ATTR, EDIT_TRIM_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_MAQAF_ATTR, "edit.maqaf", EDIT_EXPLICIT_ATTR, EDIT_EXPLICIT_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_UNICODE_LS_ATTR, "edit.unicode-ls", EDIT_EOP_ATTR, EDIT_EOP_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_SELECTED_ATTR, "edit.selected", EDIT_ATTR, EDIT_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_HTML_TAG_ATTR, "edit.html-tag", EDIT_ATTR, EDIT_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_EMPHASIZED_ATTR, "edit.emphasized", EDIT_ATTR, EDIT_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_LINKS_ATTR, "edit.links", EDIT_EMPHASIZED_ATTR, EDIT_EMPHASIZED_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_EMAIL_QUOTE1_ATTR, "edit.email-quote1", EDIT_ATTR, EDIT_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_EMAIL_QUOTE2_ATTR, "edit.email-quote2", EDIT_EMAIL_QUOTE1_ATTR, EDIT_EMAIL_QUOTE1_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_EMAIL_QUOTE3_ATTR, "edit.email-quote3", EDIT_EMAIL_QUOTE2_ATTR, EDIT_EMAIL_QUOTE2_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_EMAIL_QUOTE4_ATTR, "edit.email-quote4", EDIT_EMAIL_QUOTE3_ATTR, EDIT_EMAIL_QUOTE3_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_EMAIL_QUOTE5_ATTR, "edit.email-quote5", EDIT_EMAIL_QUOTE4_ATTR, EDIT_EMAIL_QUOTE4_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_EMAIL_QUOTE6_ATTR, "edit.email-quote6", EDIT_EMAIL_QUOTE5_ATTR, EDIT_EMAIL_QUOTE5_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_EMAIL_QUOTE7_ATTR, "edit.email-quote7", EDIT_EMAIL_QUOTE6_ATTR, EDIT_EMAIL_QUOTE6_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_EMAIL_QUOTE8_ATTR, "edit.email-quote8", EDIT_EMAIL_QUOTE7_ATTR, EDIT_EMAIL_QUOTE7_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 },
    { EDIT_EMAIL_QUOTE9_ATTR, "edit.email-quote9", EDIT_EMAIL_QUOTE8_ATTR, EDIT_EMAIL_QUOTE8_ATTR,
	MISSING_COLOR, MISSING_COLOR, 0, 0 }
};

#define ARRAY_SIZE(nm) (int)(sizeof(nm)/sizeof(nm[0]))

static const char *default_color_theme[] =
{
"menu = white, cyan",
"menu.frame = black",
"menu.selected = , black",
"menu.letter = yellow",
"menu.indicator = red",
"menubar = black",
"edit = default, default",
"edit.eop = red",
"edit.tab = red",
"edit.explicit-bidi = magenta",
"edit.maqaf = brightmagenta",
"edit.nsm = brightmagenta",
"edit.nsm.hebrew = green",
"edit.nsm.cantillation = blue",
"edit.nsm.arabic = brown",
"edit.unicode-ls = green",
"edit.wide = brightmagenta",
"edit.control = yellow",
"edit.failed-conversion = brightmagenta",
"edit.trim = bold",
"edit.wrap = bold",
"edit.selected = reverse, +bold",
"edit.html-tag = bold",
"edit.email-quote1 = bold",
"edit.emphasized = underline, +bold",
NULL
};

static const char *default_bw_theme[] =
{
"menu = reverse",
"menu.selected = normal",
"menu.letter = normal",
"menu.indicator = reverse",
"menu.indicator.selected = normal",
"edit = normal",
"edit.eop = normal",
"edit.tab = normal",
"edit.explicit-bidi = bold",
"edit.maqaf = bold",
"edit.nsm = bold",
"edit.nsm.hebrew = bold",
"edit.nsm.cantillation = bold",
"edit.nsm.arabic = bold",
"edit.unicode-ls = bold",
"edit.wide = bold",
"edit.control = bold",
"edit.failed-conversion = bold",
"edit.trim = bold",
"edit.wrap = bold",
"edit.selected = reverse",
"edit.html-tag = bold",
"edit.email-quote1 = bold",
"edit.emphasized = underline, +bold",
NULL
};

static u8string theme_name;

const char *get_theme_name()
{
    return theme_name.c_str();
}

static ATTR *get_attr_ent(int ident)
{
    static std::map<int, ATTR *> hash;

    if (hash.empty())
	for (int i = 0; i < ARRAY_SIZE(attr_table); i++)
	    hash[attr_table[i].ident] = &attr_table[i];

    std::map<int, ATTR *>::const_iterator it = hash.find(ident);
    if (it != hash.end())
	return it->second;
    else
	return NULL;
}

int get_attr(int ident)
{
    ATTR *attr = get_attr_ent(ident);
    // ASSERT(attr != NULL);
    if (attr->pair)
	return COLOR_PAIR(attr->pair) | attr->extra;
    else
	return attr->extra;
}

static int get_name_ident(const char *name)
{
    for (int i = 0; i < ARRAY_SIZE(attr_table); i++) {
	if (STREQ(attr_table[i].name, name))
	    return attr_table[i].ident;
    }
    return -1;
}

// parse_attr() parses a "color" name and puts the 
// appropriate values in an ATTR struct.

static bool parse_attr(const char *s, ATTR *attr, bool is_fg)
{
    static struct {
	const char *name;
	int color;
	int extra;
    } names_arr[] = {
	{ "black",	    COLOR_BLACK,    0 },
	{ "gray",	    COLOR_BLACK,    A_BOLD },
	{ "grey",	    COLOR_BLACK,    A_BOLD },
	{ "red",	    COLOR_RED,	    0 },
	{ "brightred",	    COLOR_RED,	    A_BOLD },
	{ "green",	    COLOR_GREEN,    0 },
	{ "brightgreen",    COLOR_GREEN,    A_BOLD },
	{ "brown",	    COLOR_YELLOW,   0 },
	{ "yellow",	    COLOR_YELLOW,   A_BOLD },
	{ "blue",	    COLOR_BLUE,	    0 },
	{ "brightblue",	    COLOR_BLUE,	    A_BOLD },
	{ "magenta",	    COLOR_MAGENTA,  0 },
	{ "brightmagenta",  COLOR_MAGENTA,  A_BOLD },
	{ "cyan",	    COLOR_CYAN,	    0 },
	{ "brightcyan",	    COLOR_CYAN,	    A_BOLD },
	{ "lightgray",	    COLOR_WHITE,    0 },
	{ "lightgrey",	    COLOR_WHITE,    0 },
	{ "white",	    COLOR_WHITE,    A_BOLD },

	{ "default",	    -1,		    0 },

	{ "",		    MISSING_COLOR,  0 },
	{ "inherit",	    MISSING_COLOR,  0 },
	
	{ "bold",	    NO_COLOR,	    A_BOLD },
	{ "underline",	    NO_COLOR,	    A_UNDERLINE },
	{ "reverse",	    NO_COLOR,	    A_REVERSE },
	{ "normal",	    NO_COLOR,	    0 },

	{ "+bold",	    SKIP_COLOR,	    A_BOLD },
	{ "+underline",	    SKIP_COLOR,	    A_UNDERLINE },
	{ "+reverse",	    SKIP_COLOR,	    A_REVERSE }
    };

    bool found = false;
    for (int i = 0; i < ARRAY_SIZE(names_arr) && !found; i++) {
	if (STREQ(s, names_arr[i].name)) {
	    if (names_arr[i].color != SKIP_COLOR) {
		if (is_fg)
		    attr->fg = names_arr[i].color;
		else
		    attr->bg = names_arr[i].color;
	    }
	    attr->extra |= names_arr[i].extra;
	    found = true;
	}
    }
    return found;
}

#define LINE_TYPE_NONE	0
#define LINE_TYPE_ATTR	1

static bool parse_line(char *ln, int *line_type, ATTR *attr)
{
    if (strchr(ln, '#'))
	*strchr(ln, '#') = '\0';

    if (strchr(ln, '=')) {
	*line_type = LINE_TYPE_ATTR;
	attr->clear();
	char *pos = strchr(ln, '=');
	u8string name = u8string(ln, pos).trim();
	if ((attr->ident = get_name_ident(name.c_str())) == -1)
	    return false;
	ln = ++pos;
	int token_no = 0;
	while (pos) {
	    u8string token;
	    pos = strchr(ln, ',');
	    if (!pos)
		token = u8string(ln);
	    else
		token = u8string(ln, pos);
	    token = token.trim();
	    if (!parse_attr(token.c_str(), attr, token_no == 0))
		return false;
	    ln = pos + 1;
	    token_no++;
	}
    } else {
	*line_type = LINE_TYPE_NONE;
    }

    return true;
}

static void clear_table()
{
    for (int i = 0; i < ARRAY_SIZE(attr_table); i++)
	attr_table[i].clear();
}

// complete_table() is called after the theme file has been parsed
// and attr_table populated. Its function is to get rid of any
// missing colors - by inheritance from the parent elements.
//
// Also, if our curses implementation doesn't support the use of
// default fg & bg colors, we use white & black instead.

static void complete_table()
{
    for (int i = 0; i < ARRAY_SIZE(attr_table); i++) {
	if (attr_table[i].fg == MISSING_COLOR) {
	    attr_table[i].fg = get_attr_ent(attr_table[i].fg_parent)->fg;
	    attr_table[i].extra |= get_attr_ent(attr_table[i].fg_parent)->extra;
	}
	if (attr_table[i].bg == MISSING_COLOR) {
	    attr_table[i].bg = get_attr_ent(attr_table[i].bg_parent)->bg;
	    // No sense in A_BOLD for background.
	    //attr_table[i].extra |= get_attr_ent(attr_table[i].bg_parent)->extra;
	}
	if (!terminal::use_default_colors) {
	    if (attr_table[i].fg == -1)
		attr_table[i].fg = COLOR_WHITE;
	    if (attr_table[i].bg == -1)
		attr_table[i].bg = COLOR_BLACK;
	}
    }
}

// allocate_color_pairs() is called after the theme file
// has been read and parsed. It does the actual color
// allocation, by calling init_pait().

static bool allocate_color_pairs(ThemeError &theme_error)
{
    int clrpr = 1; // color pair '0' is reserved and cannot be redefined.
    for (int i = 0; i < ARRAY_SIZE(attr_table); i++) {
	if (attr_table[i].fg == NO_COLOR || attr_table[i].bg == NO_COLOR) {
	    attr_table[i].pair = 0;
	} else  {
	    if (!terminal::is_color) {
		theme_error.what = ThemeError::errNoColorTerminal;
		return false;
	    }
	    bool already_allocated = false;
	    for (int j = 0; j < i; j++) {
		if (attr_table[j].fg == attr_table[i].fg
			&& attr_table[j].bg == attr_table[i].bg) {
		    attr_table[i].pair = attr_table[j].pair;
		    already_allocated = true;
		    break;
		}
	    }
	    if (!already_allocated) {
		if (clrpr > COLOR_PAIRS-1) {
		    theme_error.what = ThemeError::errNotEnoughColorPairs;
		    return false;
		}
		init_pair(clrpr, attr_table[i].fg, attr_table[i].bg);
		attr_table[i].pair = clrpr;
		clrpr++;
	    }
	}
    }
    return true;
}

bool load_theme(const char *basefilename, ThemeError &theme_error)
{
#define MAX_LINE_LEN 1024
    FILE *fp;
    u8string filename;
    theme_error.what = ThemeError::errNone;

    // First, try to load from the user directory.
    filename.cformat("%s%s", get_cfg_filename(USER_THEMES_DIR), basefilename);
    if ((fp = fopen(filename.c_str(), "r")) == NULL) {
	if (errno != ENOENT) {
	    // File exists, but some IO error occured.
	    theme_error.what = ThemeError::errIO;
	    theme_error.sys_errno = errno;
	    theme_error.filename = filename;
	    return false;
	}
	// Now try to load from the system directory.
	filename.cformat("%s%s", get_cfg_filename(SYSTEM_THEMES_DIR), basefilename);
	if ((fp = fopen(filename.c_str(), "r")) == NULL) {
	    theme_error.what = ((errno == ENOENT) ? ThemeError::errNotFound : ThemeError::errIO);
	    theme_error.sys_errno = errno;
	    theme_error.filename = filename;
	    return false;
	}
    }
    theme_error.filename = filename; // for possible future errors.

    clear_table();

    char line[MAX_LINE_LEN];
    ATTR prs_attr;
    int  line_type;
    int  line_no = 1;
    while (fgets(line, MAX_LINE_LEN, fp)) {
	if (!parse_line(line, &line_type, &prs_attr)) {
	    theme_error.what = ThemeError::errSyntax;
	    theme_error.line_no = line_no;
	    return false;
	} else if (line_type == LINE_TYPE_ATTR) {
	    ATTR *attr = get_attr_ent(prs_attr.ident);
	    attr->fg = prs_attr.fg;
	    attr->bg = prs_attr.bg;
	    attr->extra = prs_attr.extra;
	}
	line_no++;
    }
    fclose(fp);

    complete_table();
    if (allocate_color_pairs(theme_error)) {
	theme_name = basefilename;
	return true;
    } else {
	return false;
    }
#undef MAX_LINE_LEN
}

static bool load_default_theme(const char *theme_file_name,
			       const char **memory_theme,
			       ThemeError &theme_error)
{
    // First, try to load the theme from disk.
    if (!load_theme(theme_file_name, theme_error)) {
	if (theme_error.what != ThemeError::errNotFound)
	    // The theme is found on disk, be could not
	    // be processed.
	    return false;
    } else {
	return true;
    }

    // No theme found on disk. Load from memory.

    clear_table();

    ATTR prs_attr;
    int  line_type;
    int  line_no = 1;
    while (*memory_theme) {
	char line[1000];
	strcpy(line, *memory_theme);
	if (!parse_line(line, &line_type, &prs_attr)) {
	    theme_error.what = ThemeError::errSyntax;
	    theme_error.filename = "-INTERNAL-";
	    theme_error.line_no = line_no;
	    return false;
	} else if (line_type == LINE_TYPE_ATTR) {
	    ATTR *attr = get_attr_ent(prs_attr.ident);
	    attr->fg = prs_attr.fg;
	    attr->bg = prs_attr.bg;
	    attr->extra = prs_attr.extra;
	}
	memory_theme++;
	line_no++;
    }
    
    complete_table();
    allocate_color_pairs(theme_error);
    theme_name = theme_file_name;
    return true;
}

bool load_default_theme(ThemeError &theme_error)
{
    return load_default_theme(
		terminal::is_color ? "default.thm" : "default_bw.thm",
		terminal::is_color ? default_color_theme : default_bw_theme,
		theme_error);
}

#else

// Some stub functions for VERY primitive curses implementations:

int get_attr(int /*ident*/)
{
    return A_NORMAL;
}

bool load_theme(const char * /*basefilename*/, ThemeError &/*theme_error*/)
{
    return true;
}

bool load_default_theme(ThemeError &/*theme_error*/)
{
    return true;
}

const char *get_theme_name()
{
    return "";
}

#endif // HAVE_COLOR

u8string ThemeError::format() const
{
    u8string ret;
    switch (what) {
    case errSyntax:
	ret.cformat("Syntax error at line %d of %s", line_no, filename.c_str());
	break;
    case errNoColorTerminal:
	ret.cformat("Your terminal does not support colors, so I can't use theme %s", filename.c_str());
	break;
    case errNotEnoughColorPairs:
	ret.cformat("Your terminal support only %d color pairs, so I can't use theme %s", COLOR_PAIRS, filename.c_str());
	break;
    case errNotFound:
	ret.cformat("File does not exist: %s", filename.c_str());
	break;
    case errIO:
	ret.cformat("Can't open file %s: %s", filename.c_str(), strerror(sys_errno));
	break;
    case errNone:
	// silence the compiler
	break;
    }
    return ret;
}

