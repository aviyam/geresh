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

#ifndef BDE_EDITOR_H
#define BDE_EDITOR_H

#include "editbox.h"
#include "dialogline.h"
#include "statusline.h"
#include "inputline.h"
#include "speller.h"

class Menubar;
class Scrollbar;

class Editor : public Dispatcher, public EditBoxErrorListener {
public:
    enum scrollbar_pos_t { scrlbrNone, scrlbrLeft, scrlbrRight };
	
protected:
    
    EditBox wedit;
    DialogLine dialog;
    StatusLine status;
    Menubar *menubar;
    Scrollbar *scrollbar;
    Speller speller;
    SpellerWnd *spellerwnd;
    
    u8string filename;
    u8string encoding;
    u8string default_encoding;
    bool     new_flag;		     // is this a new file?
    u8string backup_suffix;

    u8string speller_encoding;
    u8string speller_cmd;
    u8string external_editor;

    unistring last_searched_string;  // for the "search next" command.

    bool      finished;		     // exec() quits when this flag is set.

    static Editor *global_instance;  // for use by the SIGHUP handler.

#ifdef HAVE_CURS_SET
    bool big_cursor;
#endif
    
    scrollbar_pos_t scrollbar_pos;
    bool syntax_auto_detection;

    //

    bool query_filename(const char *label, u8string &qry_filename,
			u8string &qry_encoding, int history_set = 0,
			InputLine::CompleteType complete = InputLine::cmpltAll);
    void show_kbd_error(const char *msg);

public:

    HAS_ACTIONS_MAP(Editor, Dispatcher);
    HAS_BINDINGS_MAP(Editor, Dispatcher);

    Editor();
    virtual ~Editor() {}

    static Editor *get_global_instance() {
	return global_instance;
    }

    //
    // Setters / Getters
    //
    const char *get_encoding() const { return encoding.c_str(); }
    const char *get_default_encoding() const { return default_encoding.c_str(); }
    const char *get_filename() const { return filename.c_str(); }
    const char *get_backup_suffix() const { return backup_suffix.c_str(); }
    const char *get_speller_encoding() const { return speller_encoding.c_str(); }
    const char *get_speller_cmd() const { return speller_cmd.c_str(); }
    void set_encoding(const char *s);
    void set_default_encoding(const char *s) { default_encoding = u8string(s); }
    void set_filename(const char *s) { filename = u8string(s); }
    void set_backup_suffix(const char *s) { backup_suffix = u8string(s); }
    void set_speller_encoding(const char *s) { speller_encoding = u8string(s); }
    void set_speller_cmd(const char *s) { speller_cmd = u8string(s); }
    u8string get_external_editor();
    void set_external_editor(const char *cmd);
    bool is_new() const { return new_flag; }
    bool is_untitled() const { return filename.empty(); }
    void set_new(bool value) { new_flag = value; }
    bool is_speller_loaded() const { return speller.is_loaded(); }
    bool in_spelling() const { return spellerwnd != NULL; }
    void set_scrollbar_pos(scrollbar_pos_t);
    scrollbar_pos_t get_scrollbar_pos() const { return scrollbar_pos; }
    void adjust_speller_cmd();
    void set_theme(const char *theme);
    INTERACTIVE void set_default_theme();
    const char *get_theme_name();
  
    //
    // Methods to access EditBox. These are used to set
    // command-line options.
    //
    void set_tab_width(int width) { wedit.set_tab_width(width); }
    void set_justification_column(int column)
	{ wedit.set_justification_column(column); }
    void toggle_auto_indent() { wedit.toggle_auto_indent(); }
    void toggle_auto_justify() { wedit.toggle_auto_justify(); }
    void toggle_formatting_marks() { wedit.toggle_formatting_marks(); }
    void go_to_line(int line) { wedit.move_absolute_line(line - 1); }
    void set_wrap_type(EditBox::WrapType value) { wedit.set_wrap_type(value); }
    void set_dir_algo(diralgo_t value) { wedit.set_dir_algo(value); }
    void set_scroll_step(int value) { wedit.set_scroll_step(value); }
    void set_smart_typing(bool value) { wedit.set_smart_typing(value); }
    void set_rfc2646_trailing_space(bool value)
	{ wedit.set_rfc2646_trailing_space(value); }
    void set_maqaf_display(EditBox::maqaf_display_t disp)
	{ wedit.set_maqaf_display(disp); }
    void set_rtl_nsm_display(EditBox::rtl_nsm_display_t disp)
	{ wedit.set_rtl_nsm_display(disp); }
    void set_undo_size_limit(size_t limit)
	{ wedit.set_undo_size_limit(limit); }
    void set_key_for_key_undo(bool value)
	{ wedit.set_key_for_key_undo(value); }
    void set_read_only(bool value)
	{ wedit.set_read_only(value); }
    void set_non_interactive_text_width(int cols)
	{ wedit.set_non_interactive_text_width(cols); }
    void enable_bidi(bool value) { wedit.enable_bidi(value); }
    void log2vis(const char *options);
    bool get_syntax_auto_detection()
	{ return syntax_auto_detection; }
    void set_syntax_auto_detection(bool value);
    void set_underline(bool value) { wedit.set_underline(value); }
    void set_visual_cursor_movement(bool value)
	{ wedit.set_visual_cursor_movement(value); }

    //
    // Interactive commands
    // 
    INTERACTIVE void show_character_info();
    INTERACTIVE void show_character_code();
    INTERACTIVE void refresh_and_center();
    INTERACTIVE void describe_key();
    INTERACTIVE void toggle_cursor_position_report();
    bool is_cursor_position_report() const;
    INTERACTIVE void quit();
    INTERACTIVE void layout_windows();
    INTERACTIVE void menu();
    INTERACTIVE void help();
    void show_help_topic(const char *topic);
    INTERACTIVE void save_file();
    INTERACTIVE void save_file_as();
    INTERACTIVE void load_file();
    INTERACTIVE void insert_file();
    INTERACTIVE void write_selection_to_file();
    INTERACTIVE void change_tab_width();
    INTERACTIVE void change_justification_column();
    INTERACTIVE void insert_unicode_char();
    INTERACTIVE void go_to_line();
    INTERACTIVE void search_forward();
    INTERACTIVE void search_forward_next();
    INTERACTIVE void change_directory();
    INTERACTIVE void toggle_arabic_shaping();
    INTERACTIVE void toggle_graphical_boxes();
    INTERACTIVE void change_scroll_step();
    void menu_set_encoding(bool default_encoding);
    INTERACTIVE void menu_set_scrollbar_none();
    INTERACTIVE void menu_set_scrollbar_left();
    INTERACTIVE void menu_set_scrollbar_right();
    INTERACTIVE void load_unload_speller();
    INTERACTIVE void spell_check_all();
    INTERACTIVE void spell_check_forward();
    INTERACTIVE void spell_check_word();
    void spell_check(Speller::splRng range);
#ifdef HAVE_CURS_SET
    INTERACTIVE void toggle_big_cursor();
#endif
    INTERACTIVE void toggle_syntax_auto_detection();
    INTERACTIVE void external_edit_prompt();
    INTERACTIVE void external_edit_no_prompt();
    void external_edit(bool prompt);

    //
    // Misc
    //
    void exec();
    void emergency_save();
    void show_file_io_error(const char *msg, const char *filename);
    bool load_file(const char *filename, const char *specified_encoding);
    bool save_file(const char *filename, const char *specified_encoding);
    bool write_selection_to_file(const char *filename,
				 const char *specified_encoding);
    bool insert_file(const char *raw_filename, const char *encoding);
    void search_forward(const unistring &search);
    void refresh(bool soft = false);
    void update_terminal(bool soft = false);
    void show_hint(const char *msg);
    void detect_syntax();

#ifdef HAVE_CURS_SET
    bool is_big_cursor() const { return big_cursor; }
    void set_big_cursor(bool value) {
	big_cursor = value;
	curs_set(value ? 2 : 1);
    }
#endif

    // EditBoxErrorListener implementation
    virtual void on_read_only_error(unichar ch);
    virtual void on_no_selection_error();
    virtual void on_no_alt_kbd_error();
    virtual void on_no_translation_table_error();
    virtual void on_cant_display_nsm_error();
};

#endif

