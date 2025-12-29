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

#include "editbox.h"
#include "editor.h"
#include "helpbox.h"
#include "dialogline.h"
#include "inputline.h"
#include "speller.h"
#include "basemenu.h"

#define ESC 27

EditBox::action_entry EditBox::actions_table[] = {
    ADD_ACTION(EditBox, key_left,
	    N_("Move a character left")),
    ADD_ACTION(EditBox, key_right,
	    N_("Move a character right")),
    ADD_ACTION(EditBox, move_previous_line,
	    N_("Move to the previous line")),
    ADD_ACTION(EditBox, move_next_line,
	    N_("Move to the next line")),
    ADD_ACTION(EditBox, move_forward_page,
	    N_("Move to the next page")),
    ADD_ACTION(EditBox, move_backward_page,
	    N_("Move to the previous page")),
    ADD_ACTION(EditBox, move_forward_char,
	    N_("Move forward a character")),
    ADD_ACTION(EditBox, move_backward_char,
	    N_("Move back a character")),
    ADD_ACTION(EditBox, move_beginning_of_line,
	    N_("Move to the beginning of the current line, logical")),
    ADD_ACTION(EditBox, move_beginning_of_visual_line,
	    N_("Move to the beginning of the current line, visual")),
    ADD_ACTION(EditBox, key_home,
	    N_("Move to the beginning of the current line")),
    ADD_ACTION(EditBox, move_end_of_line,
	    N_("Move to the end of the current line")),
    ADD_ACTION(EditBox, move_last_modification,
	    N_("Jump to the place of the last editing operation")),
    ADD_ACTION(EditBox, delete_backward_char,
	    N_("Delete the previous character")),
    ADD_ACTION(EditBox, delete_forward_char,
	    N_("Delete the character the cursor is on")),
    ADD_ACTION(EditBox, copy,
	    N_("Copy the selected text to clipboard")),
    ADD_ACTION(EditBox, cut,
	    N_("Copy the selected text to clipboard and delete it from the buffer")),
    ADD_ACTION(EditBox, paste,
	    N_("Paste the text in the clipboard into the buffer")),
    ADD_ACTION(EditBox, move_end_of_buffer,
	    N_("Move to the end of the buffer")),
    ADD_ACTION(EditBox, move_beginning_of_buffer,
	    N_("Move to the beginning of the buffer")),
    ADD_ACTION(EditBox, move_backward_word,
	    N_("Move to the beginning of the current or previous word")),
    ADD_ACTION(EditBox, move_forward_word,
	    N_("Move to the end of the current or next word")),
    ADD_ACTION(EditBox, delete_backward_word,
	    N_("Delete to the beginning of the current or previous word")),
    ADD_ACTION(EditBox, delete_forward_word,
	    N_("Delete to the end of the current or next word")),
    ADD_ACTION(EditBox, toggle_dir_algo,
	    N_("Change the algorithm used to determine the base direction of paragraphs")),
    ADD_ACTION(EditBox, set_dir_algo_unicode,
	    N_("Unicode's TR #9: First strong character determines base dir. Neutral gets LTR.")),
    ADD_ACTION(EditBox, set_dir_algo_context_strong,
	    N_("Contextual-strong: Unicode's TR #9 + neutral paras now inherit surroundings")),
    ADD_ACTION(EditBox, set_dir_algo_context_rtl,
	    N_("Contextual-rtl: like Contextual-strong, but if there's any RTL char, para is RTL")),
    ADD_ACTION(EditBox, set_dir_algo_force_ltr,
	    N_("Force LTR")),
    ADD_ACTION(EditBox, set_dir_algo_force_rtl,
	    N_("Force RTL")),
    ADD_ACTION(EditBox, toggle_wrap,
	    N_("Change the way long lines are displayed (whether to wrap or not, and how)")),
    ADD_ACTION(EditBox, set_wrap_type_at_white_space,
	    N_("Wrap lines, do not break words (just like a word processor)")),
    ADD_ACTION(EditBox, set_wrap_type_anywhere,
	    N_("Wrap lines, break words")),
    ADD_ACTION(EditBox, set_wrap_type_off,
	    N_("Don't wrap lines;you'll have to scroll horizontally to view the rest of the line")),
    ADD_ACTION(EditBox, toggle_alt_kbd,
	    N_("Toogle Hebrew keyboard emulation")),
    ADD_ACTION(EditBox, set_translate_next_char,
	    N_("Translate next character")),
    ADD_ACTION(EditBox, justify,
	    N_("Justify the current or next paragraph")),
    ADD_ACTION(EditBox, cut_end_of_paragraph,
	    N_("Cut [to] end of paragraph")),
    ADD_ACTION(EditBox, undo,
	    N_("Undo the last change")),
    ADD_ACTION(EditBox, redo,
	    N_("Redo the last change you canceled")),
    ADD_ACTION(EditBox, delete_paragraph,
	    N_("Delete the current paragraph")),
    ADD_ACTION(EditBox, toggle_primary_mark,
	    N_("Start/cancel selection")),
    ADD_ACTION(EditBox, toggle_auto_justify,
	    N_("Toggle auto-justify")),
    ADD_ACTION(EditBox, toggle_auto_indent,
	    N_("Toggle auto-indent")),
    ADD_ACTION(EditBox, toggle_formatting_marks,
	    N_("Toggle display of formatting marks (paragraph ends, explicit BiDi marks, tabs)")),
    ADD_ACTION(EditBox, toggle_rtl_nsm,
	    N_("Toggle display of Hebrew/Arabic points (off/transliterated/as-is)")),
    ADD_ACTION(EditBox, set_rtl_nsm_asis,
	    N_("Display Hebrew/Arabic points as-is (for capable terminals only)")),
    ADD_ACTION(EditBox, set_rtl_nsm_transliterated,
	    N_("Display Hebrew/Arabic points as highlighted ASCII characters")),
    ADD_ACTION(EditBox, set_rtl_nsm_off,
	    N_("Hide Hebrew/Arabic points")),
    ADD_ACTION(EditBox, toggle_maqaf,
	    N_("Toggle Hebrew maqaf highlighting and/or enable its ASCII transliteration")),
    ADD_ACTION(EditBox, set_maqaf_display_transliterated,
	    N_("Display the maqaf as ASCII dash")),
    ADD_ACTION(EditBox, set_maqaf_display_highlighted,
	    N_("Highlight the maqaf")),
    ADD_ACTION(EditBox, set_maqaf_display_asis,
	    N_("Display the maqaf as-is (for capable terminals)")),
    ADD_ACTION(EditBox, toggle_smart_typing,
	    N_("Toggle smart-typing mode: auto replace some plain characters with typographical ones")),
    ADD_ACTION(EditBox, insert_maqaf,
	    N_("Insert Hebrew maqaf")),
    ADD_ACTION(EditBox, toggle_read_only,
	    N_("Toggle read-only status of buffer")),
    ADD_ACTION(EditBox, toggle_eops,
	    N_("Change end-of-paragraphs type")),
    ADD_ACTION(EditBox, set_eops_unix,
	    N_("Set end-of-paragraphs type to Unix")),
    ADD_ACTION(EditBox, set_eops_dos,
	    N_("Set end-of-paragraphs type to DOS/Windows")),
    ADD_ACTION(EditBox, set_eops_mac,
	    N_("Set end-of-paragraphs type to Macintosh")),
    ADD_ACTION(EditBox, set_eops_unicode,
	    N_("Set end-of-paragraphs type to Unicode PS")),
    ADD_ACTION(EditBox, toggle_key_for_key_undo,
	    N_("Toggle key-for-key undo (whether to group small editing operations)")),
    ADD_ACTION(EditBox, toggle_bidi,
	    N_("Turn off/on the BiDi algorithm (useful when editing complicated bi-di texts)")),
    ADD_ACTION(EditBox, toggle_visual_cursor_movement,
	    N_("Toggle between logical and visual cursor movement")),
    ADD_ACTION(EditBox, menu_set_syn_hlt_none,  N_("Don't do syntax-highlighting")),
    ADD_ACTION(EditBox, menu_set_syn_hlt_html,  N_("Highlight HTML tags")),
    ADD_ACTION(EditBox, menu_set_syn_hlt_email, N_("Highlight lines starting with '>'")),
    ADD_ACTION(EditBox, toggle_underline,       N_("Whether to highlight *text* and _text_ on your terminal")),
    END_ACTIONS
};

binding_entry EditBox::bindings_table[] = {
    { Event(KEY_LEFT), "key_left" },
    { Event(KEY_RIGHT), "key_right" },
    { Event(KEY_UP), "move_previous_line" },
    { Event(KEY_DOWN), "move_next_line" },
    { Event(KEY_NPAGE), "move_forward_page" },
    { Event(KEY_PPAGE), "move_backward_page" },
    { Event(KEY_HOME), "key_home" },
    { Event(KEY_END), "move_end_of_line" },
    { Event(CTRL, 'b'), "move_backward_char" },
    { Event(CTRL, 'f'), "move_forward_char" },
    { Event(CTRL, 'p'), "move_previous_line" },
    { Event(CTRL, 'n'), "move_next_line" },
    { Event(CTRL, 'a'), "move_beginning_of_line" },
    { Event(CTRL, 'e'), "move_end_of_line" },
    { Event(CTRL, 'o'), "move_last_modification" },
    { Event(ALT, 'o'), "move_last_modification" },
    { Event(KEY_BACKSPACE), "delete_backward_char" },
    { Event(KEY_DC), "delete_forward_char" },
    { Event(CTRL, 'd'), "delete_forward_char" },
    { Event(ALT, 'h'), "toggle_alt_kbd" },
    { Event(KEY_F(12)), "toggle_alt_kbd" },
    { Event(ALT, '>'), "move_end_of_buffer" },
    { Event(ALT, '<'), "move_beginning_of_buffer" },
    { Event(ALT, 'b'), "move_backward_word" },
    { Event(ALT, 'f'), "move_forward_word" },
    { Event(ALT, 'F'), "toggle_formatting_marks" },
    { Event(ALT, 'n'), "toggle_rtl_nsm" },
    { Event(ALT, 'd'), "delete_forward_word" },
    { Event(ALT, 0, KEY_BACKSPACE), "delete_backward_word" },
    { Event(ALT, 't'), "toggle_dir_algo" },
    { Event(ALT, 'w'), "toggle_wrap" },
    { Event(CTRL, 'q'), "set_translate_next_char" },
    { Event(CTRL, 'j'), "justify" },
    { Event(CTRL, 'k'), "cut_end_of_paragraph" },
    { Event(CTRL, 'r'), "redo" },
    { Event(CTRL, 'u'), "undo" },
    { Event(CTRL, 'c'), "copy" },
    { Event(CTRL, 'x'), "cut" },
    { Event(CTRL, 'v'), "paste" },
    { Event(CTRL, 'y'), "delete_paragraph" },
    { Event(CTRL, '^'), "toggle_primary_mark" },
    { Event(CTRL, '@'), "toggle_primary_mark" },
    { Event(KEY_F(11)), "toggle_primary_mark" }, // cygwin
    { Event(ALT, 'J'), "toggle_auto_justify" },
    { Event(ALT, 'i'), "toggle_auto_indent" },
    { Event(ALT, 'k'), "toggle_maqaf" },
    { Event(ALT, 'q'), "toggle_smart_typing" },
    { Event(ALT, '-'), "insert_maqaf" },
    { Event(ALT, 'R'), "toggle_read_only" },
    { Event(CTRL | ALT, 'e'), "toggle_eops" },
    { Event(VIRTUAL, 2001), "set_eops_unix" },
    { Event(VIRTUAL, 2002), "set_eops_dos" },
    { Event(VIRTUAL, 2003), "set_eops_mac" },
    { Event(VIRTUAL, 2004), "set_eops_unicode" },
    { Event(VIRTUAL, 2005), "set_rtl_nsm_off" },
    { Event(VIRTUAL, 2006), "set_rtl_nsm_asis" },
    { Event(VIRTUAL, 2007), "set_rtl_nsm_transliterated" },
    { Event(VIRTUAL, 2008), "set_maqaf_display_transliterated" },
    { Event(VIRTUAL, 2009), "set_maqaf_display_highlighted" },
    { Event(VIRTUAL, 2010), "set_maqaf_display_asis" },
    { Event(VIRTUAL, 2011), "set_wrap_type_at_white_space" },
    { Event(VIRTUAL, 2012), "set_wrap_type_anywhere" },
    { Event(VIRTUAL, 2013), "set_wrap_type_off" },
    { Event(ALT, '1'), "set_dir_algo_unicode" },
    { Event(ALT, '2'), "set_dir_algo_context_strong" },
    { Event(ALT, '3'), "set_dir_algo_context_rtl" },
    { Event(ALT, '4'), "set_dir_algo_force_ltr" },
    { Event(ALT, '5'), "set_dir_algo_force_rtl" },
    { Event(VIRTUAL, 2300), "toggle_key_for_key_undo" },
    { Event(CTRL | ALT, 'b'), "toggle_bidi" },
    { Event(VIRTUAL, 4000), "menu_set_syn_hlt_none" },
    { Event(VIRTUAL, 4001), "menu_set_syn_hlt_html" },
    { Event(VIRTUAL, 4002), "menu_set_syn_hlt_email" },
    { Event(VIRTUAL, 4010), "toggle_underline" },
    { Event(ALT, 'v'), "toggle_visual_cursor_movement" },
    END_BINDINGS
};

Editor::action_entry Editor::actions_table[] = {
    ADD_ACTION(Editor, layout_windows, NULL),
    ADD_ACTION(Editor, load_file,
	    N_("Load file from disk")),
    ADD_ACTION(Editor, save_file,
	    N_("Save file to disk")),
    ADD_ACTION(Editor, save_file_as,
	    N_("Save file in another name")),
    ADD_ACTION(Editor, insert_file,
	    N_("Insert file from disk (or read from pipe) into the buffer")),
    ADD_ACTION(Editor, change_tab_width,
	    N_("Change the TAB character size")),
    ADD_ACTION(Editor, change_justification_column,
	    N_("Change the column used to justify paragraphs")),
    ADD_ACTION(Editor, insert_unicode_char,
	    N_("Insert a specific Unicode character using its known code number")),
    ADD_ACTION(Editor, go_to_line,
	    N_("Go to a specific line")),
    ADD_ACTION(Editor, search_forward,
	    N_("Search for a string, starting from the cursor")),
    ADD_ACTION(Editor, search_forward_next,
	    N_("Search for the next occurrence of the string")),
    ADD_ACTION(Editor, toggle_cursor_position_report,
	    N_("Toggle continuous display of cursor position in the status line")),
    ADD_ACTION(Editor, refresh_and_center,
	    N_("Repaint the terminal screen (if it was garbled for some reason)")),
    ADD_ACTION(Editor, show_character_code,
	    N_("Print the unicode value and UTF-8 sequence of the character the cursor is on")),
    ADD_ACTION(Editor, show_character_info,
	    N_("Print the corresponding UnicodeData.txt line of the character the cursor is on")),
    ADD_ACTION(Editor, quit,
	    N_("Quit the editor")),
    ADD_ACTION(Editor, help,
	    N_("Get help for using this editor")),
    ADD_ACTION(Editor, describe_key,
	    N_("Gives a short description for a shortcut key")),
    ADD_ACTION(Editor, change_directory,
	    N_("Change current directory")),
    ADD_ACTION(Editor, toggle_arabic_shaping,
	    N_("Toggle Arabic shaping and Lam-Alif ligature")),
    ADD_ACTION(Editor, write_selection_to_file,
	    N_("Write the selected text (or the whole text, if none selected) to a file/pipe")),
    ADD_ACTION(Editor, change_scroll_step,
	    N_("Change the scroll step (# of lines to scroll up/down when cursor at top/bottom)")),
    ADD_ACTION(Editor, load_unload_speller,
	    N_("Explicitly load or unload the speller process")),
    ADD_ACTION(Editor, spell_check_all,
	    N_("Spell check all the document")),
    ADD_ACTION(Editor, spell_check_forward,
	    N_("Spell check the document, from the cursor onward")),
    ADD_ACTION(Editor, spell_check_word,
	    N_("Spell check the word on which the cursor stands")),
    ADD_ACTION(Editor, menu,
	    N_("Activate menu")),
#ifdef HAVE_CURS_SET
    ADD_ACTION(Editor, toggle_big_cursor,
	    N_("Toogle big cursor (console only); useful if you're visually impaired")),
#endif
    ADD_ACTION(Editor, toggle_graphical_boxes,
	    N_("Use graphical characters for the menus and scrollbar")),
    ADD_ACTION(Editor, external_edit_prompt,
	    N_("Edit this file with an external editor, prompt for editor command")),
    ADD_ACTION(Editor, external_edit_no_prompt,
	    N_("Edit this file with an external editor (using previously entered command)")),
    ADD_ACTION(Editor, menu_set_scrollbar_none, NULL),
    ADD_ACTION(Editor, menu_set_scrollbar_left, NULL),
    ADD_ACTION(Editor, menu_set_scrollbar_right, NULL),
    ADD_ACTION(Editor, toggle_syntax_auto_detection, N_("Whether to try to detect HTML files and email messages")),
    ADD_ACTION(Editor, set_default_theme, NULL),
    END_ACTIONS
};

binding_entry Editor::bindings_table[] = {
#ifdef KEY_RESIZE
    { Event(KEY_RESIZE), "layout_windows" },
#endif
    { Event(KEY_F(1)), "help" },
    { Event(KEY_F(2)), "save_file" },
    { Event(CTRL, 's'), "save_file_as" },
    { Event(CTRL, 'w'), "write_selection_to_file" },
    { Event(KEY_F(3)), "load_file" },
    { Event(KEY_F(7)), "search_forward" },
    { Event(KEY_F(17)), "search_forward_next" },
    { Event(ALT, 'r'), "insert_file" },
    { Event(CTRL | ALT, 'c'), "change_directory" },
    { Event(ALT, 'x'), "quit" },
    { Event(ALT, 'X'), "quit" },
    { Event(ALT, 'c'), "toggle_cursor_position_report" },
    { Event(CTRL | ALT, 't'), "change_tab_width" },
    { Event(CTRL | ALT, 'j'), "change_justification_column" },
    { Event(CTRL | ALT, 'v'), "insert_unicode_char"},
    { Event(CTRL | ALT, 'u'), "show_character_code"},
    { Event(CTRL | ALT, 's'), "change_scroll_step"},
    { Event(ALT, 'a'), "toggle_arabic_shaping"},
    { Event(ALT, '\t'), "show_character_info"},
    { Event(CTRL, 'g'), "go_to_line"},
    { Event(CTRL, 'l'), "refresh_and_center"},
    { Event(KEY_F(4)), "describe_key"},
    { Event(KEY_F(5)), "spell_check_all" },
    { Event(KEY_F(6)), "spell_check_forward" },
    { Event(ALT, '$'), "spell_check_word" },
    { Event(ALT, 'S'), "load_unload_speller" },
    { Event(KEY_F(9)), "menu" },
    { Event(KEY_F(10)), "menu" },
    { Event(ALT, 0, KEY_F(8)), "external_edit_prompt" },
    { Event(KEY_F(8)),         "external_edit_no_prompt" },
    { Event(VIRTUAL, 3000), "toggle_big_cursor" },
    { Event(VIRTUAL, 1100), "toggle_graphical_boxes" },
    { Event(VIRTUAL, 1200), "menu_set_scrollbar_none" },
    { Event(VIRTUAL, 1201), "menu_set_scrollbar_left" },
    { Event(VIRTUAL, 1202), "menu_set_scrollbar_right" },
    { Event(VIRTUAL, 1300), "toggle_syntax_auto_detection" },
    { Event(VIRTUAL, 1400), "set_default_theme" },
    END_BINDINGS
};

DialogLine::action_entry DialogLine::actions_table[] = {
    ADD_ACTION(DialogLine, layout_windows, NULL),
    ADD_ACTION(DialogLine, cancel_modal, NULL),
    ADD_ACTION(DialogLine, refresh, NULL),
    END_ACTIONS
};

binding_entry DialogLine::bindings_table[] = {
#ifdef KEY_RESIZE
    { Event(KEY_RESIZE), "layout_windows" },
#endif
    { Event(CTRL, 'g'), "cancel_modal" },
    { Event(CTRL, 'c'), "cancel_modal" },
    { Event(0, ESC),    "cancel_modal" },
    { Event(CTRL, 'l'), "refresh" },
    END_BINDINGS
};

InputLine::action_entry InputLine::actions_table[] = {
    ADD_ACTION(InputLine, previous_completion, NULL),
    ADD_ACTION(InputLine, next_completion, NULL),
    ADD_ACTION(InputLine, end_modal, NULL),
    ADD_ACTION(InputLine, previous_history, NULL),
    ADD_ACTION(InputLine, next_history, NULL),
    END_ACTIONS
};

binding_entry InputLine::bindings_table[] = {
    { Event(0, '\r'), "end_modal" },
    { Event(0, '\t'), "next_completion" },
    { Event(ALT, '\t'), "previous_completion" },
    { Event(CTRL, 'p'), "previous_history" },
    { Event(CTRL, 'n'), "next_history" },
    { Event(KEY_UP), "previous_history" },
    { Event(KEY_DOWN), "next_history" },
    END_BINDINGS
};

HelpBox::action_entry HelpBox::actions_table[] = {
    ADD_ACTION(HelpBox, end_modal, NULL),
    ADD_ACTION(HelpBox, layout_windows, NULL),
    ADD_ACTION(HelpBox, refresh_and_center, NULL),
    ADD_ACTION(HelpBox, move_to_toc, NULL),
    ADD_ACTION(HelpBox, pop_position, NULL),
    END_ACTIONS
};

binding_entry HelpBox::bindings_table[] = {
#ifdef KEY_RESIZE
    { Event(KEY_RESIZE), "layout_windows" },
#endif
    { Event(CTRL, 'l'), "refresh_and_center"},
    { Event(KEY_F(1)),  "end_modal" },
    { Event(ALT, 'x'),  "end_modal" },
    { Event(ALT, 'X'),  "end_modal" },
    { Event(0, ESC),    "end_modal" },
    { Event(ALT, 'b'),  "pop_position" },
    { Event(ALT, 't'),  "move_to_toc" },
    END_BINDINGS
};

SpellerWnd::action_entry SpellerWnd::actions_table[] = {
    ADD_ACTION(SpellerWnd, layout_windows, NULL),
    ADD_ACTION(SpellerWnd, ignore_word, NULL),
    ADD_ACTION(SpellerWnd, abort_spelling, NULL),
    ADD_ACTION(SpellerWnd, abort_spelling_restore_cursor, NULL),
    ADD_ACTION(SpellerWnd, add_to_dict, NULL),
    ADD_ACTION(SpellerWnd, edit_replacement, NULL),
    ADD_ACTION(SpellerWnd, set_global_decision, NULL),
    ADD_ACTION(SpellerWnd, refresh, NULL),
    END_ACTIONS
};

binding_entry SpellerWnd::bindings_table[] = {
#ifdef KEY_RESIZE
    { Event(KEY_RESIZE), "layout_windows" },
#endif
    { Event(CTRL, 'g'),	"abort_spelling" },
    { Event(CTRL, 'c'),	"abort_spelling" },
    { Event(0, ESC),    "abort_spelling" },
    { Event(ALT, 'x'),	"abort_spelling_restore_cursor" },
    { Event(0, 'q'),	"abort_spelling_restore_cursor" },
    { Event(0, 'x'),	"abort_spelling_restore_cursor" },
    { Event(CTRL, 'l'), "refresh" },
    { Event(0, ' '), "ignore_word" },
    { Event(0, 'a'), "add_to_dict" },
    { Event(0, 'r'), "edit_replacement" },
    { Event(0, 'g'), "set_global_decision" },
    END_BINDINGS
};

PopupMenu::action_entry PopupMenu::actions_table[] = {
    ADD_ACTION(PopupMenu, cancel_menu, NULL),
    ADD_ACTION(PopupMenu, prev_menu, NULL),
    ADD_ACTION(PopupMenu, next_menu, NULL),
    ADD_ACTION(PopupMenu, select, NULL),
    ADD_ACTION(PopupMenu, move_previous_item, NULL),
    ADD_ACTION(PopupMenu, move_next_item, NULL),
    ADD_ACTION(PopupMenu, move_first_item, NULL),
    ADD_ACTION(PopupMenu, move_last_item, NULL),
    ADD_ACTION(PopupMenu, screen_resize, NULL),
    END_ACTIONS
};

binding_entry PopupMenu::bindings_table[] = {
#ifdef KEY_RESIZE
    { Event(KEY_RESIZE), "screen_resize" },
#endif
    { Event(KEY_F(9)),  "cancel_menu" },
    { Event(KEY_F(10)), "cancel_menu" },
    { Event(CTRL, 'c'), "cancel_menu" },
    { Event(CTRL, 'g'), "cancel_menu" },
    { Event(0, ESC),    "cancel_menu" },
    { Event(KEY_LEFT),  "prev_menu" },
    { Event(KEY_RIGHT), "next_menu" },
    { Event(0, '\r'),   "select" },
    { Event(KEY_UP),    "move_previous_item" },
    { Event(KEY_DOWN),  "move_next_item" },
    { Event(KEY_HOME),  "move_first_item" },
    { Event(KEY_END),   "move_last_item" },
    { Event(KEY_PPAGE), "move_first_item" },
    { Event(KEY_NPAGE), "move_last_item" },
    { Event(CTRL, 'p'), "move_previous_item" },
    { Event(CTRL, 'n'), "move_next_item" },
    END_BINDINGS
};

Menubar::action_entry Menubar::actions_table[] = {
    ADD_ACTION(Menubar, select, NULL),
    ADD_ACTION(Menubar, next_menu, NULL),
    ADD_ACTION(Menubar, prev_menu, NULL),
    ADD_ACTION(Menubar, end_modal, NULL),
    ADD_ACTION(Menubar, screen_resize, NULL),
    END_ACTIONS
};

binding_entry Menubar::bindings_table[] = {
#ifdef KEY_RESIZE
    { Event(KEY_RESIZE), "screen_resize" },
#endif
    { Event(KEY_F(9)),  "end_modal" },
    { Event(KEY_F(10)), "end_modal" },
    { Event(CTRL, 'c'), "end_modal" },
    { Event(CTRL, 'g'), "end_modal" },
    { Event(0, ESC),    "end_modal" },
    { Event(KEY_LEFT),  "prev_menu" },
    { Event(KEY_RIGHT), "next_menu" },
    { Event(KEY_DOWN),  "select" },
    { Event(0, '\r'),   "select" },
    END_BINDINGS
};


