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

#include <stdlib.h>
#include <errno.h>
#include <unistd.h> // chdir
#include <sys/wait.h>  // WEXITSTATUS()
#include <string.h>

#include "editor.h"
#include "menus.h"
#include "scrollbar.h"
#include "geresh_io.h"
#include "pathnames.h"
#include "themes.h"
#include "utf8.h"   // used in show_character_code()
#include "dbg.h"
#include "transtbl.h"
#include "helpbox.h"

Editor *Editor::global_instance; // for SIGHUP

#define LOAD_HISTORY		1
#define SAVEAS_HISTORY		0
#define SEARCH_HISTORY		2
#define INSERT_HISTORY		1
#define CHDIR_HISTORY		4
#define WRITESELECTION_HISTORY	5
#define SPELER_CMD_HISTORY	6
#define SPELER_ENCODING_HISTORY	7
#define EXTERNALEDITOR_HISTORY	8

// describe_key() - reads a key event from the keyboard, searches and then
// prints the description of the correspoding action.

INTERACTIVE void Editor::describe_key()
{
    dialog.show_message(_("Describe key:"));
    dialog.immediate_update();

    Event evt;
    get_next_event(evt, dialog.wnd);

    // first look for a corresponding action in Editor's action map.
    // If none was found, look in EditBox's.
    const char *action, *desc = NULL;
    if ((action = get_event_action(evt)))
	desc = get_action_description(action);
    else {
	if ((action = wedit.get_event_action(evt)))
	    desc = wedit.get_action_description(action);
    }
    dialog.show_message(desc ? _(desc)
			     : _("Sorry, no description for this key"));
}

INTERACTIVE void Editor::refresh_and_center()
{
    wedit.center_line();
    refresh();
}

void Editor::refresh(bool soft)
{
    if (!soft)
	clearok(curscr, TRUE);
    if (spellerwnd)
	spellerwnd->invalidate_view();
    if (scrollbar)
	scrollbar->invalidate_view();
    if (menubar)
	menubar->invalidate_view();
    wedit.invalidate_view();
    status.invalidate_view();
    dialog.invalidate_view();
    update_terminal(soft);
}

// show_character_code() - prints the code value and the UTF-8 sequence
// of the character on which the cursor stands.

INTERACTIVE void Editor::show_character_code()
{
    unichar ch = wedit.get_current_char();
   
    // construct a nice utf-8 representation in utf8_c_sync
    char utf8[6], utf8_c_syn[6*4 + 1];
    int nbytes = unicode_to_utf8(utf8, &ch, 1);
    for (int i = 0; i < nbytes; i++)
	sprintf(utf8_c_syn + i*4, "\\x%02X", (unsigned char)utf8[i]);

    dialog.show_message_fmt(_("Unicode value: U+%04lX (decimal: %ld), "
			      "UTF-8 sequence: %s"),
			      (unsigned long)ch, (unsigned long)ch,
			      utf8_c_syn);
}

// show_character_info() - prints information from UnicodeData.txt
// about the characetr on which the cursor stands.

INTERACTIVE void Editor::show_character_info()
{
#define MAX_LINE_LEN 1024
    unichar ch = wedit.get_current_char();

    FILE *fp;
    if ((fp = fopen(get_cfg_filename(USER_UNICODE_DATA_FILE), "r")) != NULL
	    || (fp = fopen(get_cfg_filename(SYSTEM_UNICODE_DATA_FILE), "r")) != NULL) {
	char line[MAX_LINE_LEN];
	while (fgets(line, MAX_LINE_LEN, fp)) {
	    if (strtol(line, NULL, 16) == (long)ch) {
		line[strlen(line) - 1] = '\0'; // remove LF
		dialog.show_message(line);
		break;
	    }
	}
	fclose(fp);
    } else {
	set_last_error(errno);
	show_file_io_error(_("Can't open %s: %s"),
			    get_cfg_filename(SYSTEM_UNICODE_DATA_FILE));
    }
#undef MAX_LINE_LEN
}

Editor::Editor()
    : dialog(this),
      status(this, &wedit),
      speller(*this, dialog)
{
    spellerwnd = NULL;
    wedit.set_error_listener(this);
    global_instance = this;
    set_default_encoding(DEFAULT_FILE_ENCODING);
    set_encoding(get_default_encoding());
    set_filename("");
    set_new(false);
#ifdef HAVE_CURS_SET
    big_cursor = false;
#endif

    menubar = create_main_menubar(this, &wedit);
    scrollbar = NULL;
    scrollbar_pos = scrlbrNone;
    
    // The rest is only needed if we're in interactive mode,
    // that is, not in "do_log2vis" mode.
    if (terminal::is_interactive()) {
	layout_windows();
	dialog.show_message_fmt(_("Welcome to %s version %s |"
				  "F1=Help, F9 or F10=Menu, ALT-X=Quit"),
				PACKAGE, VERSION);

	TranslationTable tbl;

	if (tbl.load(get_cfg_filename(USER_TRANSTBL_FILE))
		|| tbl.load(get_cfg_filename(SYSTEM_TRANSTBL_FILE)))
	    wedit.set_translation_table(tbl);

	if (tbl.load(get_cfg_filename(USER_REPRTBL_FILE))
		|| tbl.load(get_cfg_filename(SYSTEM_REPRTBL_FILE)))
	    wedit.set_repr_table(tbl);

	if (tbl.load(get_cfg_filename(USER_ALTKBDTBL_FILE))
		|| tbl.load(get_cfg_filename(SYSTEM_ALTKBDTBL_FILE))) {
	    wedit.set_alt_kbd_table(tbl);
	} else {
	    show_file_io_error(_("Can't load Hebrew kbd emulation %s: %s"),
				get_cfg_filename(SYSTEM_ALTKBDTBL_FILE));
	}
    }
}

// show_file_io_error() - convenience method to print I/O errors.

void Editor::show_file_io_error(const char *msg, const char *filename)
{
    u8string errmsg;
    errmsg.cformat(msg, filename, get_last_error());
    if (terminal::is_interactive())
	dialog.show_error_message(errmsg.c_str());
    else
	fatal("%s\n", errmsg.c_str());
}

#ifdef HAVE_CURS_SET
INTERACTIVE void Editor::toggle_big_cursor()
{
    set_big_cursor(!is_big_cursor());
}
#endif

INTERACTIVE void Editor::toggle_cursor_position_report()
{
    status.toggle_cursor_position_report();
}

bool Editor::is_cursor_position_report() const
{
    return status.is_cursor_position_report();
}

// quit() - interactive command to quit the editor. it first makes sure the
// user don't want to save the changes.

INTERACTIVE void Editor::quit()
{
    if (wedit.is_modified()) {
	bool canceled;
	bool save = dialog.ask_yes_or_no(_("Buffer was modified; save changes?"),
			&canceled);
	if (canceled)
	    return;
	if (save) {
	    save_file();
	    if (wedit.is_modified())
		return;
	}
    }
    finished = true; // signal exec()
}

// help() - interactive command to load and show the manual.

void Editor::show_help_topic(const char *topic)
{
    HelpBox helpbox(this, wedit);
    helpbox.layout_windows();
    if (helpbox.load_user_manual()) {
	if (topic)
	    helpbox.jump_to_topic(topic);
	helpbox.exec();
	refresh();
    }
}

INTERACTIVE void Editor::help()
{
    show_help_topic(NULL);
}

void Editor::set_scrollbar_pos(scrollbar_pos_t pos)
{
    if (terminal::is_interactive()) {
	if (pos == scrlbrNone) {
	    delete scrollbar;
	    scrollbar = NULL;
	} else {
	    if (!scrollbar)
		scrollbar = new Scrollbar();
	    wedit.sync_scrollbar(scrollbar);
	}
	scrollbar_pos = pos;
	layout_windows();
    }
}

INTERACTIVE void Editor::menu_set_scrollbar_none()
{
    set_scrollbar_pos(scrlbrNone);
}

INTERACTIVE void Editor::menu_set_scrollbar_left()
{
    set_scrollbar_pos(scrlbrLeft);
}

INTERACTIVE void Editor::menu_set_scrollbar_right()
{
    set_scrollbar_pos(scrlbrRight);
}

INTERACTIVE void Editor::menu()
{
    if (menubar)
	menubar->exec();
}

bool Editor::save_file(const char *filename, const char *specified_encoding)
{
    status.invalidate_view(); // encoding may change, so update the statusline
    unichar offending_char;
    if (!xsave_file(&wedit, filename, specified_encoding,
		get_backup_suffix(), offending_char)) {
	show_file_io_error(_("Saving %s failed: %s"), filename);
	if (offending_char)
	    wedit.move_first_char(offending_char);
	return false;
    } else {
	set_filename(filename);
	set_encoding(specified_encoding);
	set_new(false);
	wedit.set_modified(false);
	dialog.show_message(_("Saved OK"));
	return true;
    }
}

bool Editor::write_selection_to_file(const char *filename,
				     const char *specified_encoding)
{
    unichar offending_char;
    if (!xsave_file(&wedit, filename, specified_encoding,
		get_backup_suffix(), offending_char, true)) {
	show_file_io_error(_("Saving %s failed: %s"), filename);
	if (offending_char)
	    wedit.move_first_char(offending_char);
	return false;
    } else {
	dialog.show_message(_("Written OK"));
	return true;
    }
}

INTERACTIVE void Editor::write_selection_to_file()
{
    u8string qry_filename, qry_encoding;
    if (query_filename(wedit.has_selected_text()
			 ? _("Write selection to:")
			 : _("Write whole text to:"),
		       qry_filename, qry_encoding,  WRITESELECTION_HISTORY))
	write_selection_to_file(qry_filename.c_str(),
		    qry_encoding.empty() ? get_encoding() : qry_encoding.c_str());
}

void Editor::set_encoding(const char *s)
{
    encoding = u8string(s);
    status.invalidate_view();
}

bool Editor::load_file(const char *filename, const char *specified_encoding)
{
    bool is_new;
    u8string effective_encoding;

    status.invalidate_view();
    set_filename("");

    dialog.show_message(_("Loading..."));
    dialog.immediate_update();
    if (!xload_file(&wedit, filename, specified_encoding,
		    get_default_encoding(), effective_encoding, is_new, true)) {
	if (!effective_encoding.empty())
	    set_encoding(effective_encoding.c_str());
	set_new(false);
	show_file_io_error(_("Loading %s failed: %s"), filename);
	return false;
    } else {
	if (scrollbar)
	    wedit.sync_scrollbar(scrollbar);
	set_filename(filename);
	if (!is_new)
	    set_encoding(effective_encoding.c_str());
	else
	    set_encoding(specified_encoding ? specified_encoding
					    : get_default_encoding());
	set_new(is_new);
	dialog.show_message(_("Loaded OK"));
	if (get_syntax_auto_detection())
	    detect_syntax();
	else
	    wedit.set_syn_hlt(EditBox::synhltOff);
	return true;
    }
}

// insert_file() - insert file at the cursor location.

bool Editor::insert_file(const char *filename, const char *specified_encoding)
{
    bool dummy_is_new;
    u8string dummy_effective_encoding;
    status.invalidate_view();
    
    if (!xload_file(&wedit, filename, specified_encoding,
		    get_default_encoding(), dummy_effective_encoding,
		    dummy_is_new, false)) {
	show_file_io_error(_("Loading %s failed: %s"), filename);
	return false;
    } else {
	dialog.show_message(_("Inserted OK"));
	return true;
    }
}

INTERACTIVE void Editor::save_file_as()
{
    u8string qry_filename = get_filename(), qry_encoding;
    if (query_filename(_("Save file as:"), qry_filename, qry_encoding, SAVEAS_HISTORY))
	save_file(qry_filename.c_str(),
		    qry_encoding.empty() ? get_encoding() : qry_encoding.c_str());
}

INTERACTIVE void Editor::save_file()
{
    if (is_untitled())
	save_file_as();
    else
	save_file(get_filename(), get_encoding());
}

// emergency_save() - called by the SIGHUP handler to save the buffer.

void Editor::emergency_save()
{
    if (wedit.is_modified()) {
	u8string filename;
	filename.cformat("%s.save", is_untitled() ? PACKAGE : get_filename());
	save_file(filename.c_str(), get_encoding());
    }
}
	
INTERACTIVE void Editor::load_file()
{
    if (wedit.is_modified())
	if (!dialog.ask_yes_or_no(_("Buffer was modified; discard changes?")))
	    return;
    u8string qry_filename, qry_encoding;
    if (query_filename(_("Open file:"), qry_filename, qry_encoding, LOAD_HISTORY))
	load_file(qry_filename.c_str(),
		    qry_encoding.empty() ? NULL : qry_encoding.c_str());
}

INTERACTIVE void Editor::insert_file()
{
    u8string qry_filename, qry_encoding;
    if (query_filename(_("Insert file:"), qry_filename, qry_encoding, INSERT_HISTORY))
	insert_file(qry_filename.c_str(),
			qry_encoding.empty() ? NULL : qry_encoding.c_str());
}

INTERACTIVE void Editor::change_directory()
{
    u8string qry_dirname, dummy;
    if (query_filename(_("Change directory to:"), qry_dirname, dummy,
	    CHDIR_HISTORY, InputLine::cmpltDirectories)) {
	if (chdir(qry_dirname.c_str()) == -1) {
	    set_last_error(errno);
	    show_file_io_error(_("Can't chdir to %s: %s"), qry_dirname.c_str());
	}
    }
}

void Editor::set_theme(const char *theme)
{
    ThemeError theme_error;
    bool rslt;
    if (!theme)
	rslt = load_default_theme(theme_error);
    else
	rslt = load_theme(theme, theme_error);
    if (!rslt)
	show_file_io_error(_("Error: %s"), theme_error.format().c_str());
    refresh();
}

void Editor::set_default_theme()
{
    set_theme(NULL);
}

const char *Editor::get_theme_name()
{
    return ::get_theme_name();
}

void Editor::menu_set_encoding(bool default_encoding)
{
    unistring answer = dialog.query(
	_("Enter encoding name (do 'iconv -l' at the shell for a list):"));
    if (!answer.empty()) {
	if (default_encoding)
	    set_default_encoding(u8string(answer).c_str());
	else
	    set_encoding(u8string(answer).c_str());
    }
}

// extract_encoding() - takes the filename that the user has entered,
// of the form "/path/to/filename -encoding", and returns the "encoding"
// part. it also removes it from the filename.

static u8string extract_encoding(u8string &filename)
{
    u8string encoding;
    int pos = filename.rindex(' ');
    if (pos != -1 && (filename[pos + 1]  == '-' || filename[pos + 1] == '+')) {
	encoding.assign(filename, pos + 2, filename.size() - pos - 2);
	// we've found it. now remove the encoding part from the filename itself
	while (pos >= 0 && filename[pos] == ' ')
	    --pos;
	filename.erase(filename.begin() + pos + 1, filename.end());
    }
    return encoding;
}

// query_filename() - prompt the user for a filename. 

bool Editor::query_filename(const char *label, u8string &qry_filename,
			    u8string &qry_encoding, int history_set,
			    InputLine::CompleteType complete)
{
    unistring uni_filename;
    uni_filename.init_from_filename(qry_filename.c_str());
    // we feed dialog.query() not qry_filename but uni_filename.
    // this extra conversion is needed because qry_filename may not be
    // a valid UTF-8 string (this is possible since using UTF-8 for
    // filenames is only a convention).
    unistring answer = dialog.query(label, uni_filename, history_set, complete);
    qry_filename.init_from_unichars(answer);
    // extract encoding, but only if it's not a pipe.
    if (!qry_filename.empty() && qry_filename[0] != '|' && qry_filename[0] != '!')
	qry_encoding = extract_encoding(qry_filename);
    else
	qry_encoding = "";
    if (qry_filename == "~")
	qry_filename = "~/";
    expand_tilde(qry_filename);
    return !qry_filename.empty();
}

INTERACTIVE void Editor::change_tab_width()
{
    int num = dialog.get_number(_("Enter tab width:"), wedit.get_tab_width());
    if (num <= 80) // arbitrary sanity check
	set_tab_width(num);
}

INTERACTIVE void Editor::change_justification_column()
{
    int num = dialog.get_number(_("Enter justification column:"),
				    wedit.get_justification_column());
    set_justification_column(num);
}

INTERACTIVE void Editor::change_scroll_step()
{
    int num = dialog.get_number(_("Enter scroll step:"), wedit.get_scroll_step());
    wedit.set_scroll_step(num);
}

INTERACTIVE void Editor::insert_unicode_char()
{
    unistring answer = dialog.query(
		_("Enter unicode hex value (or end in '.' for base 10):"));
    if (answer.empty())
	return;
    u8string numstr;
    numstr.init_from_unichars(answer);
    errno = 0;
    char *endp;
    unichar ch = strtol(numstr.c_str(), &endp,
			strchr(numstr.c_str(), '.') ? 10 : 16);
    if (!errno && (*endp == '.' || *endp == '\0'))
	wedit.insert_char(ch);
}

void Editor::log2vis(const char *options)
{
    wedit.log2vis(options);
}

INTERACTIVE void Editor::toggle_arabic_shaping()
{
    terminal::do_arabic_shaping = !terminal::do_arabic_shaping;
    wedit.reformat();
    if (terminal::is_interactive())
	refresh();
}

INTERACTIVE void Editor::toggle_graphical_boxes()
{
    terminal::graphical_boxes = !terminal::graphical_boxes;
    if (terminal::is_interactive())
	refresh();
}

void Editor::detect_syntax()
{
#define NMHAS(EXT) (strstr(get_filename(), EXT))
    if (NMHAS(".htm") || NMHAS(".HTM"))
	wedit.set_syn_hlt(EditBox::synhltHTML);
    else if (NMHAS(".letter") || NMHAS(".article") || NMHAS(".followup") ||
	     NMHAS(".eml") || NMHAS("SLRN") || NMHAS("pico") || NMHAS("mutt"))
	// the above strings were taken from Vim's cfg.
	wedit.set_syn_hlt(EditBox::synhltEmail);
    else
	wedit.detect_syntax();
#undef NMHAS
}

void Editor::set_syntax_auto_detection(bool v)
{
    syntax_auto_detection = v;
    if (v)
	detect_syntax();
}

INTERACTIVE void Editor::toggle_syntax_auto_detection()
{
    set_syntax_auto_detection(!get_syntax_auto_detection());
}

INTERACTIVE void Editor::go_to_line()
{
    bool canceled;
    int num = dialog.get_number(_("Go to line #:"), 0, &canceled);
    if (!canceled)
	go_to_line(num);
}

void Editor::search_forward(const unistring &search)
{
    if (!wedit.search_forward(search))
	show_kbd_error(_("Not found"));
    last_searched_string = search;
}

INTERACTIVE void Editor::search_forward()
{
    bool alt_kbd = wedit.get_alt_kbd();
    unistring search = dialog.query(_("Search forward:"),
				    last_searched_string, SEARCH_HISTORY,
				    InputLine::cmpltOff, &alt_kbd);
    wedit.set_alt_kbd(alt_kbd);
    if (!search.empty())
	search_forward(search);
}
    
INTERACTIVE void Editor::search_forward_next()
{
    if (!last_searched_string.empty())
	search_forward(last_searched_string);
    else
	search_forward();
}

u8string Editor::get_external_editor()
{
    if (!external_editor.empty())
	return external_editor;

    // User hasn't specified an editor, so we figure one out ourselves:
    // First try $EDITOR, them gvim, then vim, then ...
    if (getenv("EDITOR") && *getenv("EDITOR"))
	return getenv("EDITOR");
    if (terminal::under_x11() && has_prog("gvim"))
	return "gvim -f"; // `-f' = not to fork and detach
    if (has_prog("vim"))
	return "vim";
    if (has_prog("emacs"))
	return "emacs";
    if (has_prog("pico"))
	return "pico";
    return "vi";
}

void Editor::set_external_editor(const char *cmd)
{
    external_editor = cmd;
}

INTERACTIVE void Editor::external_edit_prompt()
{
    external_edit(true);
}

INTERACTIVE void Editor::external_edit_no_prompt()
{
    external_edit(false);
}

// external_edit() launches an external editor to edit the file
// and then reload it.

void Editor::external_edit(bool prompt)
{
    // Step 1: we may first need to save the file.

    if (is_untitled()) {
	if (dialog.ask_yes_or_no(_("First I must save this buffer as a file; is it OK with you?")))
	    save_file();
    } else if (wedit.is_modified()) {
	if (dialog.ask_yes_or_no(_("Buffer was modified and I must save it first; is it OK with you?")))
	    save_file();
    }
    if (is_untitled() || wedit.is_modified())
	return;

    // Step 2: get the editor command

    cstring command = get_external_editor();
    if (prompt) {
	unistring inpt = dialog.query(_("External editor:"),
				      command.c_str(),
				      EXTERNALEDITOR_HISTORY,
				      InputLine::cmpltAll);
	command = u8string(inpt).trim();
	if (command.empty()) // user aborted
	    return;
	set_external_editor(command.c_str());
    }

    // Step 3: write the cursor location into a temporary file,
    // pointed to by $GERESH_CURSOR_FILE
   
    cstring cursor_file;
    //cursor_file = tmpnam(NULL); // gives linker warning!
    cursor_file.cformat("%s/geresh-%ld-cursor.tmp",
	    getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp",
	    (long)getpid());

    Point cursor;
    wedit.get_cursor_position(cursor);

    FILE *cursor_fh;
    if ((cursor_fh = fopen(cursor_file.c_str(), "w")) != NULL) {
	fprintf(cursor_fh, "%d %d\n%s\n",
		cursor.para + 1, cursor.pos + 1, get_filename());
	fclose(cursor_fh);

	cstring nameval;
	nameval.cformat("GERESH_CURSOR_FILE=%s", cursor_file.c_str());
	putenv(strdup(nameval.c_str()));
    }

    // Step 4: adjust the editor command so the line number is passed.

    if (command.index("vim") != -1) {
	// With vim we not only pass the column number in addition
	// to the line number, but we also pass the necessary
	// command to write the cursor position back into
	// $GERESH_CURSOR_FILE.
	cstring basefilename = get_filename();
	if (basefilename.index('/') != -1)
	    basefilename = basefilename.substr(basefilename.rindex('/')+1);
	cstring col_command;
	if (cursor.pos != 0)
	    col_command.cformat("-c \"normal %dl\"", cursor.pos);
	command.cformat("%s -c %d -c \"normal 0\" %s "
			"-c \"au BufUnload %s call system('echo '.line('.').' '.col('.').' >%s')\"",
			    command.c_str(),
			    cursor.para + 1,
			    col_command.c_str(),
			    basefilename.c_str(),
			    cursor_file.c_str());
    }
    else if (command.index("vi") != -1 ||
	     command.index("pico") != -1 ||
	     command.index("nano") != -1 ||
	     command.index("emacs") != -1 ||
	     command.index("mined") != -1)
    {
	// The above editors are known to accept a "+line" parameter.
	command.cformat("%s +%d", command.c_str(), cursor.para + 1);
    }

    command += " \"";
    command += get_filename();
    command += "\"";

    // Step 5: execute the editor.

    cstring msg;
    msg.cformat("Executing: %s", command.c_str());
    dialog.show_message(msg.c_str());
    update_terminal();
    
    endwin();
    int status = system(command.c_str());
    doupdate();
   
    // Step 6: reload the file.

    // I can't just pass 'get_filename()' to load_file(), because
    // get_filename() returns a pointer to a string which load_file()
    // modifies (via set_filename()) ...
    load_file(u8string(get_filename()).c_str(),
	      u8string(get_encoding()).c_str());
   
    // Step 7: read the cursor position from $GERESH_CURSOR_FILE.

    if ((cursor_fh = fopen(cursor_file.c_str(), "r")) != NULL) {
	int line_no, col_no;
	fscanf(cursor_fh, "%d %d", &line_no, &col_no);
	cursor.para = line_no - 1;
	cursor.pos  = col_no  - 1;
	wedit.set_cursor_position(cursor);
	fclose(cursor_fh);
    } else {
	wedit.set_cursor_position(cursor);
    }
    unlink(cursor_file.c_str());

    if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
	msg.cformat(_("Error: command \"%s\" could not be found"),
			get_external_editor().c_str());
	dialog.show_message(msg.c_str());
    }

    refresh_and_center();
}

// layout_windows() - is called every time the window changes its size.

INTERACTIVE void Editor::layout_windows()
{
    int cols, rows;
    getmaxyx(stdscr, rows, cols);

    int speller_height = 0;

    if (spellerwnd) {
	// determine the height of the speller window 
	// based on the screen size.
	if (rows <= 10)
	    speller_height = 2;
	else if (rows <= 16)
	    speller_height = 4;
	else if (rows <= 20)
	    speller_height = 6;
	else if (rows <= 26)
	    speller_height = 8;
	else if (rows <= 36)
	    speller_height = 11;
	else
	    speller_height = 13;
	
	spellerwnd->resize(speller_height, cols, rows - 2 - speller_height, 0);
    }

    if (menubar)
	menubar->resize(1, cols, 0, 0);
#define MENUHEIGHT (menubar ? 1 : 0)

    if (scrollbar)
	scrollbar->resize(rows - 2 - MENUHEIGHT - speller_height, 1,
			  MENUHEIGHT, (scrollbar_pos == scrlbrLeft) ? 0 : cols - 1);
#define SCROLLBARWIDTH (scrollbar ? 1 : 0)

    wedit.resize(rows - 2 - MENUHEIGHT - speller_height,
		 cols - SCROLLBARWIDTH, MENUHEIGHT,
		 (scrollbar_pos == scrlbrLeft) ? 1 : 0);
    status.resize(1, cols, rows - 2, 0);
    dialog.resize(1, cols, rows - 1, 0);

    update_terminal();
}

// update_terminal() - updates the screen.

void Editor::update_terminal(bool soft)
{
    // for every widget that's dirty, call its update() method to update
    // stdscr. If any was dirty, call doupdate() to update the physical
    // screen.

    bool need_dorefresh = false;

    if (scrollbar && scrollbar->is_dirty()) {
    	scrollbar->update();
	need_dorefresh = true;
    }

    if (menubar && menubar->is_dirty()) {
    	menubar->update();
	need_dorefresh = true;
    }
    
    if (status.is_dirty()) {
    	status.update();
	need_dorefresh = true;
    }

    if (dialog.is_dirty()) {
    	dialog.update();
	need_dorefresh = true;
    }
    
    if (wedit.is_dirty()) {
	wedit.update();
	need_dorefresh = true;
    } else if (need_dorefresh) {
	// make sure wedit gets the cursor even if some
	// of the other widgets get drawn.
	wedit.update_cursor();
    }

    if (spellerwnd) {
	if (spellerwnd->is_dirty()) {
	    spellerwnd->update();
	    need_dorefresh = true;
	} else if (need_dorefresh) {
	    // spellerwnd always gets the cursor.
	    spellerwnd->update_cursor();
	}
    }

    if (!soft && need_dorefresh)
	doupdate();
}

// Editor::exec() - the main event loop. it reads events and either handles
// them or send them on to the editbox.

void Editor::exec()
{
    finished = false;
    while (!finished) {
	Event evt;
	update_terminal();
	get_next_event(evt, wedit.wnd);
	dialog.clear_transient_message();
	if (!evt.is_literal())
	    if (handle_event(evt))
		continue;
	wedit.handle_event(evt);
	if (scrollbar)
	    wedit.sync_scrollbar(scrollbar);
    }
}

// load_unload_speller() - interactively loads and unloads a speller
// process. When the user interactively loads a speller, he is asked to
// specify the speller-command and the speller-encoding.

INTERACTIVE void Editor::load_unload_speller()
{
    if (speller.is_loaded()) {
	speller.unload();
    } else {
	// We're about to load the speller. query the user for cmd / encoding.
	unistring cmd = dialog.query(_("Enter speller command:"),
		    get_speller_cmd(), SPELER_CMD_HISTORY, InputLine::cmpltAll);
	if (cmd.empty())
	    return;
	if (cmd.index(u8string("-a")) == -1) {
	    if (!dialog.ask_yes_or_no(
		_("There's no `-a' option in the command, proceed anyway?")))
		return;
	}
	unistring encoding = dialog.query(_("Enter speller encoding:"),
		    get_speller_encoding(), SPELER_ENCODING_HISTORY);
	if (encoding.empty())
	    return;
	set_speller_cmd(u8string(cmd).c_str());
	set_speller_encoding(u8string(encoding).c_str());
	speller.load(get_speller_cmd(), get_speller_encoding());
    }
    status.invalidate_view();
}

// adjust_speller_cmd() - if the user hasn't specified a speller command,
//			  figure it out ourselves.
//			  Prefer: multispell, then ispell, and finally aspell.
//			  Use the ISO-8859-8 encoding for Hebrew spell-checkers.

void Editor::adjust_speller_cmd()
{
    bool no_cmd      = (*get_speller_cmd() == '\0');
    bool no_encoding = (*get_speller_encoding() == '\0');

    if (no_cmd) {
	bool has_multispell = has_prog("multispell");
	bool has_ispell	    = has_prog("ispell");
	bool has_aspell	    = !has_ispell && has_prog("aspell"); // optimization: search aspell
								 // only if no ispell found.
	if (has_multispell) {
	    u8string cmd;
	    if (has_ispell || has_aspell) {
		cmd = "LC_ALL=C multispell -a -n";
		if (!has_ispell && has_aspell)
		    cmd += " --ispell-cmd=aspell";
	    } else {
		cmd = "LC_ALL=C hspell -a -n";
	    }
	    set_speller_cmd(cmd.c_str());
	    set_speller_encoding("ISO-8859-8");
	} else {
	    if (!has_ispell && has_aspell)
		set_speller_cmd("aspell -a");
	    else
		set_speller_cmd("ispell -a");
	    if (no_encoding)
		set_speller_encoding("ISO-8859-1");
	}
    } else if (no_encoding) {
	if (strstr(get_speller_cmd(), "hspell") || strstr(get_speller_cmd(), "multispell"))
	    set_speller_encoding("ISO-8859-8");
	else
	    set_speller_encoding("ISO-8859-1");
    }
}

INTERACTIVE void Editor::spell_check_all()
{
    spell_check(Speller::splRngAll);
}

INTERACTIVE void Editor::spell_check_forward()
{
    spell_check(Speller::splRngForward);
}

INTERACTIVE void Editor::spell_check_word()
{
    spell_check(Speller::splRngWord);
}

void Editor::spell_check(Speller::splRng range)
{
    if (!spellerwnd) {
	spellerwnd = new SpellerWnd(*this);
	layout_windows();
    }

    // if the speller is not yet loaded, load it with the default
    // parameters.
    if (!speller.is_loaded()) {
	speller.load(get_speller_cmd(), get_speller_encoding());
	status.invalidate_view();
	update_terminal(); // immediately update the status line
    }

    if (speller.is_loaded())
	speller.spell_check(range, wedit, *spellerwnd);    

    // The speller window exists during the spell check only. we
    // delete it afterwards.
    delete spellerwnd;
    spellerwnd = NULL;
    layout_windows();
}

// show_hint() is used to print the popdown menu hints. Screen is updated
// only after doupdate()

void Editor::show_hint(const char *msg)
{
    dialog.show_message(msg);
    dialog.update();
}

// show_kbd_error() - prints a transient error message and rings the bell.
// it is usually used for "keyboard" errors and the bell is supposed to
// catch the users attention.

void Editor::show_kbd_error(const char *msg)
{
    // we don't use show_error_message() because we want the
    // message to disappear at the next event.
    dialog.show_message(msg);
    Widget::signal_error();
}

void Editor::on_read_only_error(unichar ch)
{
    unistring quit_chars;
    // when the buffer is in read-only mode, we let the user exit
    // by pressing 'q'.
    quit_chars.init_from_utf8(_("qQ"));
    if (quit_chars.has_char(ch))
	quit();
    else	
	show_kbd_error(_("Buffer is read-only"));
}

void Editor::on_no_selection_error()
{
    show_kbd_error(_("No text is selected"));
}

void Editor::on_no_alt_kbd_error()
{
    show_kbd_error(_("No alternate keyboard (Hebrew) was loaded"));
}

void Editor::on_no_translation_table_error()
{
    show_kbd_error(_("No translation table was loaded"));
}

void Editor::on_cant_display_nsm_error()
{
    show_kbd_error(_("Terminal can't display non-spacing marks (like Hebrew points)"));
}

