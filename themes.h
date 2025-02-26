#ifndef BDE_THEMES_H
#define BDE_THEMES_H

#include <config.h>

#define MENUBAR_ATTR		    10
#define MENU_ATTR		    11
#define MENU_SELECTED_ATTR	    12
#define MENU_FRAME_ATTR		    13
#define MENU_LETTER_ATTR	    14
#define MENU_LETTER_SELECTED_ATTR   15
#define MENU_INDICATOR_ATTR	    16
#define MENU_INDICATOR_SELECTED_ATTR 17
#define STATUSLINE_ATTR		    18
#define EDIT_ATTR		    19
#define DIALOGLINE_ATTR		    20
#define SCROLLBAR_ATTR		    21
#define SCROLLBAR_THUMB_ATTR	    22
#define EDIT_FAILED_CONV_ATTR	    23
#define EDIT_CONTROL_ATTR	    24
#define EDIT_EOP_ATTR		    25
#define EDIT_EXPLICIT_ATTR	    26
#define EDIT_NSM_ATTR		    27
#define EDIT_NSM_HEBREW_ATTR	    28
#define EDIT_NSM_CANTILLATION_ATTR  29
#define EDIT_NSM_ARABIC_ATTR	    30
#define EDIT_TAB_ATTR		    31
#define EDIT_WIDE_ATTR		    32
#define EDIT_TRIM_ATTR		    33
#define EDIT_WRAP_ATTR		    34
#define EDIT_MAQAF_ATTR		    35
#define EDIT_UNICODE_LS_ATTR	    36
#define EDIT_SELECTED_ATTR	    37
#define EDIT_HTML_TAG_ATTR	    38
#define EDIT_EMPHASIZED_ATTR	    39
#define EDIT_LINKS_ATTR		    40
#define EDIT_EMAIL_QUOTE1_ATTR	    101
#define EDIT_EMAIL_QUOTE2_ATTR	    102
#define EDIT_EMAIL_QUOTE3_ATTR	    103
#define EDIT_EMAIL_QUOTE4_ATTR	    104
#define EDIT_EMAIL_QUOTE5_ATTR	    105
#define EDIT_EMAIL_QUOTE6_ATTR	    106
#define EDIT_EMAIL_QUOTE7_ATTR	    107
#define EDIT_EMAIL_QUOTE8_ATTR	    108
#define EDIT_EMAIL_QUOTE9_ATTR	    109

struct ThemeError {
    enum { errNone, errNotFound, errIO,
	   errSyntax, errNoColorTerminal, errNotEnoughColorPairs } what;
    int sys_errno;  // for errIO
    int line_no;    // for errSytax
    u8string filename;
    u8string format() const;
};

int get_attr(int ident);

#ifdef HAVE_COLOR
# define contains_color(attr) PAIR_NUMBER(attr)
#else
# define contains_color(attr) 0
#endif

bool load_theme(const char *basefilename, ThemeError &theme_error);
bool load_default_theme(ThemeError &theme_error);
const char *get_theme_name();

#endif

