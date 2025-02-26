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

#ifndef BDE_EDITBOX_H
#define BDE_EDITBOX_H

#include <vector>
#include "directvect.h"

#include "widget.h"
#include "bidi.h"
#include "transtbl.h"
#include "undo.h"
#include "point.h"


// End-of-paragraph type
enum eop_t { eopNone, eopUnix, eopDOS, eopMac, eopUnicode };

// A "Paragraph" is a fundamental text element in our editor widget,
// EditBox.  EditBox stores the text in paragraphs. Paragraphs correspond
// to "lines" in traditional unix text editors, but we reserve the term
// "lines" to screen lines, that is, a single row on the screen.

class Paragraph {

public:
    
    typedef DirectVector<idx_t> IdxArray;

    unistring str; // the text itself

    // The text is wrapped into several lines to fit the screen width.
    // Since this wrapping process is costly (calculating widths, etc), we
    // store the resulting line breaks in "line_breaks" and re-apply the
    // wrapping process only upon text modification.

    IdxArray line_breaks;

    // The end-of-paragraph character (CR, LF, etc) is not stored directly
    // in "str", but we keep a record of it in "eop".

    eop_t eop;
   
    // "individual_base_dir" holds the directionality of the paragraph. It
    // is updated when the text is modified.

    direction_t individual_base_dir;

    // "contextual_base_dir" holds the directionality of the paragraph
    // taking into account its surrounding paragraphs (that is, its
    // context). "individual_base_dir", too, is used in calculating it.
    // EditBox uses this variable, mainly through is_rtl(), to reorder and
    // render the text.

    direction_t contextual_base_dir;

public:

    int breaks_count() const { return line_breaks.size(); }
    direction_t base_dir() const { return contextual_base_dir; }
    bool is_rtl() const { return contextual_base_dir == dirRTL; }

    Paragraph() {
	line_breaks.push_back(0);
	individual_base_dir = contextual_base_dir = dirN;
	eop = eopNone;
    }

    void determine_base_dir(diralgo_t dir_algo)
    {
	individual_base_dir = BiDi::determine_base_dir(str.begin(),
						       str.len(), dir_algo);
	// If we're not asked to use the contextual algorithm, then
	// calculating "contextual_base_dir", which is what EditBox looks
	// at, is a simple assignment.
	if (dir_algo != algoContextStrong && dir_algo != algoContextRTL)
	    contextual_base_dir = individual_base_dir;
    }
};

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))

// EditBox is one widget that doesn't visually notify the user about
// changes in its state. Such chores are the responsibility of a
// higher-level class, like the status line or the editor. In order to let
// other objects know about state changes and various errors, EditBox let's
// them register themselves as "listeners". This concept is similar to
// Java's listeners and adapters.

class EditBoxStatusListener {
public:
    virtual void on_wrap_change() {}
    virtual void on_modification_change() {}
    virtual void on_selection_change() {}
    virtual void on_position_change() {}
    virtual void on_auto_indent_change() {}
    virtual void on_auto_justify_change() {}
    virtual void on_dir_algo_change() {}
    virtual void on_alt_kbd_change() {}
    virtual void on_formatting_marks_change() {}
    virtual void on_read_only_change() {}
    virtual void on_smart_typing_change() {}
    virtual void on_rtl_nsm_change() {}
    virtual void on_maqaf_change() {}
    virtual void on_translation_mode_change() {}
};

class EditBoxErrorListener {
public:
    virtual void on_read_only_error(unichar ch = 0) {}
    virtual void on_no_selection_error() {}
    virtual void on_no_alt_kbd_error() {}
    virtual void on_no_translation_table_error() {}
    virtual void on_cant_display_nsm_error() {}
};

// A CombinedLine object addresses a line. Remember that a "line" in our
// terminology is a screen line. Therefore CombineLine is a combination of
// a paragraph number (para) and a line number inside this paragraph
// (inner_line).

class CombinedLine {

public:
	
    int para;
    int inner_line;

public:

    CombinedLine() {
	para = 0;
	inner_line = 0;
    }
    CombinedLine(int para, int inner_line) {
	this->para = para;
	this->inner_line = inner_line;
    }
    CombinedLine(const CombinedLine &other) {
	para = other.para;
	inner_line = other.inner_line;
    }
    bool operator< (const CombinedLine &other) const {
	return para < other.para
		    || (para == other.para && inner_line < other.inner_line);
    }
    bool operator== (const CombinedLine &other) const {
	return para == other.para
		    && inner_line == other.inner_line;
    }
    bool operator> (const CombinedLine &other) const {
	return !(*this < other
		    || *this == other);
    }
    void swap(CombinedLine &other) {
	CombinedLine tmp = other;
	other = *this;
	*this = tmp;
    }

    // Some operations, like calculating the difference between two lines,
    // were meant be used on lines that are close together. the is_near()
    // method helps us avoid this problem by telling us that the lines are
    // far apart. (the exact number is unimportant, but it should be of
    // the same magnitude as the window height.)

    bool is_near(const CombinedLine &other) const {
	int diff = para - other.para;
	return diff < 40 && diff > -40;
    }    
};

// EditBox is the central widget in our editor. It stores the text and does
// the actual editing.
//
// The text is stored as a vector of pointers to Paragraphs. Professional
// editors are not built that way. What I did here is not an effective
// data-structure for an editor.
//
// It's too late to "fix" this as it would require major modifications.
// This editor started as a tiny project and from the start I intended it
// for simple editing tasks, like composing mail messages and nothing more.
// 
// Data Transfer:
//
// EditBox does not handle loading and saving files. This is the task of an
// I/O module. To allow other objects to access EditBox's textual data, we
// provide a data transfer interface:
// 
// start_data_transfer(direction);
// transfer_data(buffer);
// end_data_transfer(); 

class Scrollbar;

class EditBox : public Widget {

public:

    // the following typedef's are shorthands for STL's types. There're
    // all used by the reordering algorithm (BiDi related).

    typedef DirectVector<level_t>	LevelsArray;
    typedef Paragraph::IdxArray		IdxArray;
    typedef DirectVector<attribute_t>	AttributeArray;
    typedef DirectVector<int>		IntArray;

    // EditBox supports two types of wrap (in addition to no-wrap):
    // wrpAnywhere, that breaks lines even inside words; and
    // wrpAtWhiteSpace, that doesn't break words (unless they're longer
    // than the window width).

    enum WrapType { wrpOff, wrpAnywhere, wrpAtWhiteSpace };

    // How to display the Hebrew maqaf?
    // 1. mqfAsis ("as is") is used when the user has Unicode fonts -- they
    // usually have maqaf.
    // 2. 8-bit fonts don't have it, that's why we may have to convert it
    // to ASCII's dash (mqfTransliterate).
    // 3. mqfHighlighed makes the maqaf very visible, in case the user
    // wants to know whether the document is dominated by maqafs.

    enum maqaf_display_t { mqfAsis, mqfTransliterated, mqfHighlighted };

    // How to display RTL NSMs?
    // 1. rtlnsmOff -- don't print them.
    // 2. rtlnsmTransliterated -- use ASCII characters to represent them.
    // 3. rtlnsmAsis -- if the user is lucky to have a system (terminal +
    // font) that is capable of displaying Hebrew points, print them as-is.

    enum rtl_nsm_display_t { rtlnsmOff, rtlnsmTransliterated, rtlnsmAsis };

    enum syn_hlt_t { synhltOff, synhltHTML, synhltEmail };
    
    // In the update() method we draw only the region that needs to be
    // redrawn.
    // rgnCursor -- only position the cursor; rgnCurrent -- draw the
    // current paragraph and position the cursor; rgnRange -- draw a range
    // of paragraphs; argnAll -- draw all visible paragraphs.

    enum region { rgnNone = 0, rgnCursor = 1, rgnCurrent = 2,
		  rgnRange = 4, rgnAll = 8 };

    enum data_transfer_direction { dataTransferIn, dataTransferOut };

protected:

    // for translating the next char typed.
    static TranslationTable transtbl;

    // ASCII representations of various non-displayable characters and
    // concepts, like line continuation.
    static TranslationTable reprtbl;

    // Alternate keyboard -- usually the user's national language.
    static TranslationTable altkbdtbl;
    
    EditBoxStatusListener *status_listener;
    EditBoxErrorListener  *error_listener;

#define NOTIFY_CHANGE(event) \
    do { \
	if (status_listener) \
	    status_listener->on_ ## event ## _change(); \
    } while (0)

#define NOTIFY_ERROR(event) \
    do { \
	if (error_listener) \
	    error_listener->on_ ## event ## _error(); \
    } while (0)

#define NOTIFY_ERROR_ARG(event, arg) \
    do { \
	if (error_listener) \
	    error_listener->on_ ## event ## _error(arg); \
    } while (0)
    
    // The text itself: a vector of pointers to Paragraphs
    std::vector<Paragraph *> paragraphs;

    // The cursor position
    Point cursor;
    
    UndoStack undo_stack;
    unistring clipboard;

    // Margins make it easier for us to implement features such as line
    // numbering.  Currently EditBox only uses "margin_after" to reserve
    // one column for the '$' character (the indicator that, in no-wrap
    // mode, tells the user that only part of the line is shown), and for
    // the cursor, when it stands past the last character in the line.
    //
    // Other editors call these "left_margin" and "right_margin", but since
    // this is a BiDi editor, where paragraphs may be RTL oriented, I use
    // "before" and "after" to mean "left" and "right", respectively, for
    // LTR paragraphs, and "right" and "left", respectively, for RTL
    // paragraphs.

    int margin_before;
    int margin_after;

    // top_line refers to the first visible line in our window.
    CombinedLine top_line;

    // The "selection" (shown highlighted when active) stretches from the
    // curser position till this primary_mark.
    Point primary_mark;

    // When the user moves the cursor vertically, the editor tries to
    // reposition the cursor on the same visual column. This is not always
    // possible, e.g. when that column would be inside a TAB segment, or
    // when the new line is not long enough to contain this column.
    // optinal_vis_colum holds the column on which the cursor _should_
    // stand.
    int optimal_vis_column;

    // Has the buffer been modified?
    bool modified;

    // justification_column holds the maximum line length to use when
    // justifying paragraphs.
    idx_t justification_column;

    // when rfc2646_trailing_space is true, we keep a trailing space on
    // each line when justifying paragraphs.
    bool  rfc2646_trailing_space;

    // When the user moves the cursor past the top or the bottom of the window,
    // EditBox scrolls scroll_step lines up or down.
    int scroll_step;

    // update_region tells the update() method which part of the window to
    // paint. it's a bitmask of 'enum region'.
    int update_region;
    // when drawing a range of paragraphs (rgnRange), update_from and
    // update_to denote the region.
    int update_from;
    int update_to;

    // Modes
    bool alt_kbd;	    // is the alternative keyboard active? (cf. altkbdtbl)
    bool auto_justify;	    // is auto-justify mode active?
    bool auto_indent;	    // whether to auto-indent lines.
    bool translation_mode;  // translate next char? (cf. transtbl)
    bool read_only;	    // is the user allowed to make changes to the buffer?
    bool smart_typing;	    // replace simple ASCII chars with fancy ones.

    // Display
    WrapType wrap_type;
    diralgo_t dir_algo;
    int tab_width;
    bool show_explicits;	    // display BiDi formatting codes?
    bool show_paragraph_endings;    // display paragraph endings?
    bool show_tabs;
    rtl_nsm_display_t rtl_nsm_display;
    maqaf_display_t maqaf_display;
    
    syn_hlt_t syn_hlt;
    bool underline_hlt;

    int non_interactive_text_width;
    // on resize event, compare new width to old_width
    // and wrap lines only if they're different.
    int old_width;

    bool bidi_enabled;

    bool visual_cursor_movement;
    
    // Where the last edit operation took place?
    Point last_modification;

    // We emulate some emacs commands (e.g. C-k) with these
    // flags. When prev_command_type is cmdtpKill, we know
    // we need to append to the clipboard.
    enum CommandType { cmdtpUnknown, cmdtpKill };
    CommandType prev_command_type, current_command_type;

private:
    
    struct _t_data_transfer {
	bool in_transfer;
	data_transfer_direction dir;
	Point cursor_origin;
	bool skip_undo;
	bool clear_modified_flag;
	bool at_eof;
	bool prev_is_cr;
	int ntransferred_out;
	int ntransferred_out_max;
    } data_transfer;

public:

    void set_read_only(bool value);
    bool is_read_only() const { return read_only; }
    INTERACTIVE void toggle_read_only() { set_read_only(!is_read_only()); }

    void set_cursor_position(const Point &point);
    void get_cursor_position(Point& point) const { point = cursor; }

    void set_maqaf_display(maqaf_display_t disp);
    maqaf_display_t get_maqaf_display() const {	return maqaf_display; }
    INTERACTIVE void toggle_maqaf();
    INTERACTIVE void set_maqaf_display_transliterated() {
	set_maqaf_display(mqfTransliterated);
    }
    INTERACTIVE void set_maqaf_display_highlighted() {
	set_maqaf_display(mqfHighlighted);
    }
    INTERACTIVE void set_maqaf_display_asis() {
	set_maqaf_display(mqfAsis);
    }

    void set_smart_typing(bool val);
    bool is_smart_typing() const { return smart_typing; }
    INTERACTIVE void toggle_smart_typing()
	{ set_smart_typing(!is_smart_typing()); }

    bool set_rtl_nsm_display(rtl_nsm_display_t disp);
    rtl_nsm_display_t get_rtl_nsm_display() const
	{ return rtl_nsm_display; }
    INTERACTIVE void toggle_rtl_nsm();
    INTERACTIVE void set_rtl_nsm_off() {
	set_rtl_nsm_display(rtlnsmOff);
    }
    INTERACTIVE void set_rtl_nsm_asis() {
	set_rtl_nsm_display(rtlnsmAsis);
    }
    INTERACTIVE void set_rtl_nsm_transliterated() {
	set_rtl_nsm_display(rtlnsmTransliterated);
    }

    void set_formatting_marks(bool value);
    bool has_formatting_marks() const {	return show_paragraph_endings; }
    INTERACTIVE void toggle_formatting_marks();

    void set_tab_width(int value);
    int get_tab_width() const { return tab_width; }

    void set_modified(bool value);
    bool is_modified() const { return modified; }

    void set_auto_indent(bool value);
    bool is_auto_indent() const { return auto_indent; }
    INTERACTIVE void toggle_auto_indent()
	{ set_auto_indent(!is_auto_indent()); }

    void set_auto_justify(bool value);
    bool is_auto_justify() const { return auto_justify; }
    INTERACTIVE void toggle_auto_justify()
	{ set_auto_justify(!is_auto_justify()); }

    void set_justification_column(int value);
    int get_justification_column() const { return justification_column; }
    void set_rfc2646_trailing_space(bool value) { rfc2646_trailing_space = value; }
   
    void set_translation_table(const TranslationTable &tbl) { transtbl = tbl; }
    void set_translation_mode(bool value);
    bool in_translation_mode() const { return translation_mode; }
    INTERACTIVE void set_translate_next_char() { set_translation_mode(true); }

    void set_scroll_step(int value);
    int get_scroll_step() const { return scroll_step; }

    void set_wrap_type(WrapType value);
    WrapType get_wrap_type() const { return wrap_type; }
    INTERACTIVE void toggle_wrap();
    INTERACTIVE void set_wrap_type_at_white_space() {
	set_wrap_type(wrpAtWhiteSpace);
    }
    INTERACTIVE void set_wrap_type_anywhere() {
	set_wrap_type(wrpAnywhere);
    }
    INTERACTIVE void set_wrap_type_off() {
	set_wrap_type(wrpOff);
    }
    void set_non_interactive_text_width(int cols)
	{ non_interactive_text_width = cols; }

    void set_dir_algo(diralgo_t value);
    diralgo_t get_dir_algo() const { return dir_algo; }
    INTERACTIVE void toggle_dir_algo();
    INTERACTIVE void set_dir_algo_unicode()	{ set_dir_algo(algoUnicode); }
    INTERACTIVE void set_dir_algo_force_ltr()	{ set_dir_algo(algoForceLTR); }
    INTERACTIVE void set_dir_algo_force_rtl()	{ set_dir_algo(algoForceRTL); }
    INTERACTIVE void set_dir_algo_context_strong() { set_dir_algo(algoContextStrong); }
    INTERACTIVE void set_dir_algo_context_rtl()	{ set_dir_algo(algoContextRTL); }

    void set_alt_kbd_table(const TranslationTable &tbl) { altkbdtbl = tbl; }
    void set_alt_kbd(bool val);
    bool get_alt_kbd() const { return alt_kbd; }
    INTERACTIVE void toggle_alt_kbd();

    void set_primary_mark(const Point &point);
    void set_primary_mark() { set_primary_mark(cursor); }
    void unset_primary_mark();
    bool is_primary_mark_set() const { return primary_mark.para != -1; }
    bool has_selected_text() const { return is_primary_mark_set(); }
    INTERACTIVE void toggle_primary_mark();

    void set_repr_table(const TranslationTable &tbl)
	{ reprtbl = tbl; }
    void set_status_listener(EditBoxStatusListener *obj)
	{ status_listener = obj; }
    void set_error_listener(EditBoxErrorListener *obj)
	{ error_listener = obj; }
    void set_undo_size_limit(size_t limit)
	{ undo_stack.set_size_limit(limit); }
    void set_key_for_key_undo(bool value)
	{ undo_stack.set_merge(!value); }
    bool is_key_for_key_undo() const
	{ return !undo_stack.is_merge(); }
    INTERACTIVE void toggle_key_for_key_undo();
    void sync_scrollbar(Scrollbar *scrollbar);

    void enable_bidi(bool value);
    bool is_bidi_enabled() const { return bidi_enabled; }
    INTERACTIVE void toggle_bidi();

    void set_syn_hlt(syn_hlt_t sh);
    syn_hlt_t get_syn_hlt() const
	{ return syn_hlt; }
    void detect_syntax();

    INTERACTIVE void menu_set_syn_hlt_none();
    INTERACTIVE void menu_set_syn_hlt_html();
    INTERACTIVE void menu_set_syn_hlt_email();

    void set_underline(bool v);
    bool get_underline() const
	{ return underline_hlt; }
    INTERACTIVE void toggle_underline();
    
    void set_visual_cursor_movement(bool v);
    bool get_visual_cursor_movement() const { return visual_cursor_movement; }
    INTERACTIVE void toggle_visual_cursor_movement();

protected:

    int parags_count() const { return paragraphs.size(); }

    inline Paragraph *curr_para() { return paragraphs[cursor.para]; };

public:
    
    HAS_ACTIONS_MAP(EditBox, Widget);
    HAS_BINDINGS_MAP(EditBox, Widget);

    ///////////////////////////////////////////////////////////////////////
    //                          Initialization
    ///////////////////////////////////////////////////////////////////////

    EditBox();
    virtual ~EditBox();
    void new_document();

    ///////////////////////////////////////////////////////////////////////
    //                          Data Transfer
    ///////////////////////////////////////////////////////////////////////

public:

    void start_data_transfer(data_transfer_direction dir,
			     bool new_document = false,
			     bool selection_only = false);
    int transfer_data(unichar *buf, int len);
    void end_data_transfer(); 

protected:

    int transfer_data_in(unichar *data, int len);
    int transfer_data_out(unichar *buf, int len);
    bool is_in_data_transfer() const;

    ///////////////////////////////////////////////////////////////////////
    //                          Movement
    ///////////////////////////////////////////////////////////////////////

public:

    INTERACTIVE void move_forward_char();
    INTERACTIVE void move_backward_char();
    INTERACTIVE void move_next_line();
    INTERACTIVE void move_previous_line();
    INTERACTIVE void move_forward_page();
    INTERACTIVE void move_backward_page();
    INTERACTIVE void move_beginning_of_line();
    INTERACTIVE void move_end_of_line();
    INTERACTIVE void move_beginning_of_buffer();
    INTERACTIVE void move_end_of_buffer();
    INTERACTIVE void move_backward_word();
    INTERACTIVE void move_forward_word();
    INTERACTIVE void move_last_modification();
    INTERACTIVE void key_left();
    INTERACTIVE void key_right();
    INTERACTIVE void key_home();
    INTERACTIVE void center_line();
    void move_first_char(unichar ch);
    bool search_forward(unistring str);
    unichar get_current_char();
    void move_absolute_line(int line);

    // Visual cursor movement:
    INTERACTIVE void move_forward_visual_char();
    INTERACTIVE void move_backward_visual_char();
    INTERACTIVE void move_beginning_of_visual_line();

protected:

    void post_horizontal_movement();
    void post_vertical_movement();
    bool is_at_end_of_buffer();
    bool is_at_beginning_of_buffer();
    bool is_at_beginning_of_word();
    void move_relative_line(int diff);
    void add_rows_to_line(CombinedLine &line, int rows);
    int lines_diff(CombinedLine L1, CombinedLine L2);
    void scroll_to_cursor_line();
    int calc_inner_line();
    int calc_vis_column();
    void move_to_vis_column(int column);
    void invalidate_optimal_vis_column()
	{ optimal_vis_column = -1; }
    bool valid_optimal_vis_column() const
	{ return optimal_vis_column != -1; }
    int get_effective_scroll_step() const
	{ return MAX(1, MIN(window_height() / 2 + 0 , scroll_step)); }
    
    // Visual cursor movement related:
    bool is_at_end_of_screen_line();
    bool is_at_first_screen_line();
    bool is_at_last_screen_line();

    ///////////////////////////////////////////////////////////////////////
    //                          Modification
    ///////////////////////////////////////////////////////////////////////

public:

    INTERACTIVE void delete_paragraph();
    INTERACTIVE void cut_end_of_paragraph();
    INTERACTIVE void delete_forward_word();
    INTERACTIVE void delete_backward_word();
    INTERACTIVE void delete_forward_char();
    INTERACTIVE void delete_backward_char();
    void delete_text(int len, bool skip_undo = false,
		     unistring *alt_deleted = NULL);
    void insert_char(unichar ch);
    void insert_text(const unichar *str, int len, bool skip_undo = false);
    void insert_text(const unistring &str, bool skip_undo = false)
	{ insert_text(str.begin(), str.len(), skip_undo); }
    void replace_text(const unichar *str, int len, int delete_len,
		      bool skip_undo = false);
    void replace_text(const unistring &str, int delete_len,
		      bool skip_undo = false)
	{ replace_text(str.begin(), str.len(), delete_len, skip_undo); }
    INTERACTIVE void copy();
    INTERACTIVE void cut();
    INTERACTIVE void paste();
    INTERACTIVE void undo();
    INTERACTIVE void redo();
    INTERACTIVE void justify();
    INTERACTIVE void insert_maqaf();
    INTERACTIVE void toggle_eops();
    void set_eops(eop_t new_eop);
    eop_t get_dominant_eop() { return paragraphs[0]->eop; }
    INTERACTIVE void set_eops_unix() { set_eops(eopUnix); }
    INTERACTIVE void set_eops_dos() { set_eops(eopDOS); }
    INTERACTIVE void set_eops_mac() { set_eops(eopMac); }
    INTERACTIVE void set_eops_unicode() { set_eops(eopUnicode); }
    void log2vis(const char *options);
    void key_enter();
    void key_dash();

protected:

    void post_modification();
    void post_para_modification(Paragraph &p);
    void undo_op(UndoOp *opp);
    void redo_op(UndoOp *opp);
    void calc_contextual_dirs(int min_para, int max_para, bool update_display);
    inline void set_contextual_dir(Paragraph &p, direction_t dir,
				   bool update_display);
    int calc_distance(Point p1, Point p2);
    void copy_text(int len, bool append = false);
    void cut_or_copy(bool just_copy);
    inline unichar get_eop_char(eop_t eop);
    inline unichar get_curr_eop_char();

    ////////////////////////////////////////////////////////////////////////////
    //                          Rendering
    ////////////////////////////////////////////////////////////////////////////
public:

    virtual void update();
    virtual void update_cursor() { request_update(rgnCursor); update(); }
    virtual bool is_dirty() const { return update_region != rgnNone; }
    virtual void invalidate_view() { request_update(rgnAll); }
    void reformat();

protected:

    // get_char_width() returns the width of a characetr, but since
    // this character may be part of a LAM-ALEF ligature, we need
    // to keep state information between calls - wdstate.
    struct wdstate {
	bool may_start_lam_alef;
	void clear() {
	    may_start_lam_alef = false;
	}
	wdstate() {
	    clear();
	}
    };

    int get_text_width() const;
    // :TODO: try to make this inline. FreeBSD's compiler complains...
    /*inline*/
	int get_char_width(unichar ch, int pos,
		    wdstate *stt = NULL, bool visual = false);
    int get_str_width(const unichar *str, idx_t len, bool visual = false);
    int get_rev_str_width(const unichar *str, idx_t len);
    void wrap_para(Paragraph &para);
    void rewrap_all();
    void request_update(region rgn);
    void request_update(int lo, int hi);

    struct _t_cache {
	int owner_para;
	LevelsArray levels;
	IdxArray position_L_to_V;
	IdxArray position_V_to_L;
	AttributeArray attributes;
	unistring vis;

	void invalidate() {
	    owner_para = -1;
	}
	bool owned_by(int para) {
	    return owner_para == para;
	}
	void set_owner(int para) {
	    owner_para = para;
	}
    } cache;

    virtual void redraw_paragraph(
			    Paragraph &p,
			    int window_start_line,
			    bool only_cursor,
			    int para_num
			);

    void EditBox::redraw_unwrapped_paragraph(
			    Paragraph &p,
			    int window_start_line,
			    bool only_cursor,
			    int para_num,
			    LevelsArray &levels,
			    IdxArray& position_L_to_V,
			    IdxArray& position_V_to_L,
			    AttributeArray& attributes,
			    bool eop_is_selected
			);

    void EditBox::redraw_wrapped_paragraph(
			    Paragraph &p,
			    int window_start_line,
			    bool only_cursor,
			    int para_num,
			    LevelsArray &levels,
			    IdxArray& position_L_to_V,
			    IdxArray& position_V_to_L,
			    AttributeArray& attributes,
			    bool eop_is_selected
			);
	
    void draw_unistr(const unichar *str, idx_t len,
		     attribute_t *attributes = NULL, int *tab_widths = NULL);
    void calc_tab_widths(const unichar *str, idx_t len,
			 bool rev, IntArray &tab_widths);
    void draw_rev_unistr(const unichar *str, idx_t len,
			 attribute_t *attributes = NULL);
    unichar get_char_repr(unichar ch);
    void put_unichar_attr_at(int line, int col, unichar ch, int attr);
    void draw_eop(int y, int x, Paragraph &p, bool selected);
    virtual void do_syntax_highlight(const unistring &str, 
	    AttributeArray &attributes, int para_num);

    ///////////////////////////////////////////////////////////////////////
    //                          Misc
    ///////////////////////////////////////////////////////////////////////

public:

    virtual bool handle_event(const Event &evt);
    virtual void resize(int lines, int columns, int y, int x);

public:

    int get_number_of_paragraphs() const
	{ return parags_count(); }
    const unistring &get_paragraph_text(int i) {
	return paragraphs[i]->str;
    }
};

#endif

