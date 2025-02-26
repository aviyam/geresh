#include <unistd.h>	// exec, pipe, fork...
#include <algorithm>	// std::sort

#include "speller.h"
#include "mk_wcwidth.h"
#include "converters.h"
#include "editor.h"
#include "dialogline.h"
#include "dbg.h"

// A Correction class encapsulates an incorrect word, its position
// in the text, and a list of seggested corrections.

class Correction {

    bool valid;

public:

    Correction(const char *s, int aLine);
    bool is_valid() { return valid; }

    unistring incorrect;
    // we also save a version of the word represented
    // in the speller encoding, so we don't have to convert
    // the word back later.
    cstring   incorrect_original;
    std::vector<unistring> suggestions;

    // The position of the incorrect word: line number and offset within.

    int line;
    int offset;

    // hspell sometimes returns spelling-hints (short textual explanation
    // of why the word is incorrect).

    unistring hint;
    
    void add_hint(const unistring &s) {
	if (!hint.empty())
	    hint.push_back('\n');
	hint.append(s);
    }
};

// A Corrections class holds a list of Correction objects pertaining
// to one paragraph of text.

class Corrections {

    std::vector<Correction *> array;

    // A function object to sort the Correction objects by their offset
    // within the paragraph.
    struct cmp_corrections {
	bool operator() (const Correction *a, const Correction *b) const {
	    return a->offset < b->offset;
	}
    };

public:

    Corrections() {}
    ~Corrections();

    void clear();
    void add(Correction *crctn);
    bool empty() const	{ return array.empty(); }
    int size() const	{ return (int)array.size(); }
    Correction *operator[] (int i)
			{ return array[i]; }

    // The speller (e.g. hspell) may not report incorrect words in
    // the order in which they appear in the paragraph. This is because
    // hspell delegates the work to [ia]spell after it finishes reporting
    // the incorrect Hebrew words. However, since we want to present
    // the user the words in the right order, we have to sort them first.

    void sort() {
	std::sort(array.begin(), array.end(), cmp_corrections());
    }
};

Corrections::~Corrections()
{
    clear();
}

void Corrections::clear()
{
    for (int i = 0; i < size(); i++)
	delete array[i];
    array.clear();
}

void Corrections::add(Correction *crctn)
{
    array.push_back(crctn);
}

// A Correction constructor parses an ispell-a line.
// 
// The detailed description of the ispell-a protocol can be found
// in the ispell man page. In short, when the speller finds an incorrect
// word and has some spell suggestions, it returns:
//
// [&?] incorrect-word count offset: word, word, word, word
//
// When it has no suggestions, it returns:
// 
// # <<incorrect>> <<offset>>
//
// If the protocol-line does not conform to the above syntaxes, we
// ignore it and mark the object as invalid.

Correction::Correction(const char *s, int aLine)
{
    if (*s != '&' && *s != '?' && *s != '#') {
	valid = false;
	return;
    }
    valid  = true;
    line   = aLine;
    offset = -1;

    bool has_suggestions = (*s != '#');
    
    const char *pos, *start;
    start = pos = s + 2;
    while (*pos != ' ')
	pos++;
    incorrect.init_from_utf8(start, pos);

    offset = strtol(pos, (char **)&pos, 10);
    if (has_suggestions)
	offset = strtol(pos, (char **)&pos, 10);
    // we sent the speller lines prefixed with "^", so we need
    // to decrease by one.
    offset--;

    // the following post[1,2] tests are needed because
    // hspell returns "?" instead of "#" when there are
    // no suggestions.
    if (has_suggestions && pos[1] && pos[2]) {
	unistring word;
	do {
	    start = pos += 2;
	    while (*pos && *pos != ',')
		pos++;
	    word.init_from_utf8(start, pos);
	    suggestions.push_back(word);
	} while (*pos);
    }
}

//////////////////////////// SpellerWnd //////////////////////////////////

SpellerWnd::SpellerWnd(Editor &aApp) :
    app(aApp)
{
    create_window();
    label.highlight();
    label.set_text(_("Speller Results"));
    // The following are the keys the user presses to select
    // a spelling suggestion. These can be modified using gettext's
    // message catalogs.
    word_keys.init_from_utf8(
	_("1234567890:;<=>@bcdefhijklmnopqstuvwxyz[\\]^_`"
	  "BCDEFHIJKLMNOPQSTUVWXYZ{|}~"));
}

void SpellerWnd::resize(int lines, int columns, int y, int x)
{
    Widget::resize(lines, columns, y, x);
    label.resize(1, columns, y, x);
    editbox.resize(lines - 1, columns, y + 1, x);
}

void SpellerWnd::update()
{
    label.update();
    editbox.update();
}

bool SpellerWnd::is_dirty() const
{
    return label.is_dirty() || editbox.is_dirty();
}

void SpellerWnd::invalidate_view()
{
    label.invalidate_view();
    editbox.invalidate_view();
}

INTERACTIVE void SpellerWnd::layout_windows()
{
    app.layout_windows();
}

INTERACTIVE void SpellerWnd::refresh()
{
    app.refresh();
}

void SpellerWnd::clear()
{
    editbox.new_document();
}

void SpellerWnd::append(const unistring &us)
{
    editbox.insert_text(us);
}

void SpellerWnd::append(const char *s)
{
    unistring us;
    us.init_from_utf8(s);
    editbox.insert_text(us);
}

void SpellerWnd::end_menu(MenuResult result)
{
    menu_result = result;
    finished = true;
}

INTERACTIVE void SpellerWnd::ignore_word()
{
    end_menu(splIgnore);
}

INTERACTIVE void SpellerWnd::add_to_dict()
{
    end_menu(splAdd);
}

INTERACTIVE void SpellerWnd::edit_replacement()
{
    end_menu(splEdit);
}

INTERACTIVE void SpellerWnd::abort_spelling()
{
    end_menu(splAbort);
}

INTERACTIVE void SpellerWnd::abort_spelling_restore_cursor()
{
    end_menu(splAbortRestoreCursor);
}

INTERACTIVE void SpellerWnd::set_global_decision()
{
    global_decision = true;
    editbox.set_read_only(false);
    editbox.move_beginning_of_buffer();
    append(_("--GLOBAL DECISION--\n"));
    editbox.set_read_only(true);
}

// handle_event() - 
//
// A typical SpellerWnd window displays:
// 
// (1) begging (2) begin (3) begun (4) bagging (5) beguine
//
// In brackets are the keys the user presses to choose a
// spelling suggestion. We handle these keys here. 

bool SpellerWnd::handle_event(const Event &evt)
{
    if (Widget::handle_event(evt))
	return true;
    if (evt.is_literal()) {
	int idx = word_keys.index(evt.ch);
	if (idx != -1 && idx < (int)correction->suggestions.size()) {
	    suggestion_choice = idx;
	    end_menu(splChoice);
	}
	return true;
    }
    return editbox.handle_event(evt);
}

// exec_correction_menu() - Setup the SpellerWnd contents and then
// execute a modal menu (using an event loop). It returns the user's
// action.

MenuResult SpellerWnd::exec_correction_menu(Correction &crctn)
{
    // we save the Correction object in a member variable because
    // other methods (e.g. handle_event) use it.
    correction = &crctn;

    u8string title;
    title.cformat(_("Suggestions for '%s'"),
		     u8string(correction->incorrect).c_str());
    label.set_text(title.c_str());

    editbox.set_read_only(false);
    clear();
    for (int i = 0; i < (int)correction->suggestions.size()
			&& i < word_keys.len(); i++)
    {
	u8string utf8_word(correction->suggestions[i]);
	u8string utf8_key(word_keys.substr(i, 1));
	u8string word_tmplt;
	if (i != 0)
	    append("\xC2\xA0 "); // UNI_NO_BREAK_SPACE
	word_tmplt.cformat(_("(%s)\xC2\xA0%s"),
			     utf8_key.c_str(), utf8_word.c_str()); 
	append(word_tmplt.c_str());
    }
    if (correction->suggestions.empty())
	append(_("No suggestions for this word."));
    append("\n\n");
    if (!correction->hint.empty()) {
	append(correction->hint);
	append("\n\n");
    }
    append(_("[SPC to leave unchanged, 'a' to add to private dictionary, "
	     "'r' to edit word, 'q' to exit and restore cursor, ^C to "
	     "exit and leave cursor, or one of the above characters "
	     "to replace.  'g' to make your decision global.]"));
    editbox.set_read_only(true);
    editbox.move_beginning_of_buffer();
	    
    global_decision = false;
    finished = false;
    while (!finished) {
	Event evt;
	app.update_terminal();
	get_next_event(evt, editbox.wnd);
	handle_event(evt);
    }
    return menu_result;
}

///////////////////////////// Speller ////////////////////////////////////

#define SPELER_REPLACE_HISTORY	110

// the following UNLOAD_SPELLER routine is a temporary hack to
// a pipe problem (see TODO).
static Speller *global_speller_instance = NULL;
void UNLOAD_SPELLER()
{
    if (global_speller_instance)
	global_speller_instance->unload();
}

// replace_table is a hash-table that matches any incorrect word
// with its correct spelling. It is used to implement the "Replace
// All" function. Also, when the value of the key is the empty
// string, it means to ignore the word (that's how "Ignore All" is
// implemented).

std::map<unistring, unistring> replace_table;

Speller::Speller(Editor &aApp, DialogLine &aDialog) :
    app(aApp),
    dialog(aDialog)
{
    loaded = false;
    global_speller_instance = this;
}

// load() - loads the speller. it forks and execs the speller. it setups
// pipes for communication.
//
// Warning: the code is not foolproof! it expects the child process to
// print an identity string. if the child prints nothing, this function
// hangs!

bool Speller::load(const char *cmd, const char *encoding)
{
    if (is_loaded())
	return true;

    conv_to_speller =
	    ConverterFactory::get_converter_to(encoding);
    conv_from_speller =
	    ConverterFactory::get_converter_from(encoding);
    if (!conv_to_speller || !conv_from_speller) {
	dialog.show_message_fmt(_("Can't find converter '%s'"), encoding);
	return false;
    }
    conv_to_speller->enable_ilseq_repr();

    dialog.show_message(_("Loading speller..."));
    dialog.immediate_update();

    if (pipe(fd_to_spl) < 0 || pipe(fd_from_spl) < 0) {
	dialog.show_message(_("pipe() error"));
	return false;
    }
    pid_t pid;
    if ((pid = fork()) < 0) {
	dialog.show_message(_("fork() error"));
	return false;
    }
    if (pid == 0) {
	DISABLE_SIGTSTP();
	// we're in the child.
	dup2(fd_to_spl[0],   STDIN_FILENO);
	dup2(fd_from_spl[1], STDOUT_FILENO);
	dup2(fd_from_spl[1], STDERR_FILENO);

	close(fd_from_spl[0]); close(fd_to_spl[0]);
	close(fd_from_spl[1]); close(fd_to_spl[1]);

	execlp("/bin/sh", "sh", "-c", cmd, NULL);

	// write the error back to the parent
	u8string err;
	err.cformat(_("Error %d (%s)\n"), errno, strerror(errno));
	write(STDOUT_FILENO, err.c_str(), err.size());
	exit(1);
    }

    dialog.show_message(_("Waiting for the speller to finish loading..."));
    dialog.immediate_update();

    u8string identity = read_line();
    
    if (identity.c_str()[0] != '@') {
	dialog.show_message_fmt(_("Error: Not a speller: %s"),
				identity.c_str());
	unload();
	return false;
    } else {   
	// display the speller identity for a brief moment.
	dialog.show_message(identity.c_str());
	dialog.immediate_update();
	sleep(1);
	write_line("@ActivateExtendedProtocol\n"); // for future extensions :-)
	dialog.show_message(_("Speller loaded OK."));
	loaded = true;
	return true;
    }
}

void Speller::unload()
{
    if (loaded) {
	close(fd_from_spl[0]); close(fd_to_spl[0]);
	close(fd_from_spl[1]); close(fd_to_spl[1]);
	delete conv_to_speller;
	delete conv_from_speller;
	loaded = false;
    }
}

// convert_from_unistr() and convert_to_unistr() convert from unicode
// to the speller encoding and vice versa.

void convert_from_unistr(cstring &cstr, const unistring &str,
			 Converter *conv)
{
    char    *buf = new char[str.len() * 6 + 1]; // Max UTF-8 seq is 6.
    unichar *us_p = (unichar *)str.begin();
    char    *cs_p = buf;
    conv->convert(&cs_p, &us_p, str.len());
    cstr = cstring(buf, cs_p);
}

void convert_to_unistr(unistring &str, const cstring &cstr,
		       Converter *conv)
{
    str.resize(cstr.size());
    unichar *us_p = (unichar *)str.begin();
    char    *cs_p = (char *)&*cstr.begin(); // convert iterator to pointer
    conv->convert(&us_p, &cs_p, cstr.size());
    str.resize(us_p - str.begin());
}

void Speller::add_to_dictionary(Correction &correction)
{
    replace_table[correction.incorrect] = unistring(); // "Ignore All"
    cstring cstr;
    cstr.cformat("*%s\n", correction.incorrect_original.c_str());
    write_line(cstr.c_str());
    write_line("#\n");
}

// interactive_correct() - let the user interactively correct the
// spelling mistakes. For every incorrect word, it:
//
// 1. highlights the word
// 2. calls exec_correction_menu() to display the menu
// 3. acts based on the user action.
//
// returns 'false' if the user aborts.

bool Speller::interactive_correct(Corrections &corrections,
				  EditBox &wedit,
				  SpellerWnd &splwnd,
				  bool &restore_cursor)
{
    for (int cur_crctn = 0; cur_crctn < corrections.size(); cur_crctn++)
    {
	Correction &correction = *corrections[cur_crctn];

	MenuResult menu_result;
	unistring  replace_with;
	
	if (replace_table.find(correction.incorrect) != replace_table.end()) {
	    replace_with = replace_table[correction.incorrect];
	    menu_result  = splEdit;
	} else {
	    // highlight the word
	    wedit.unset_primary_mark();
	    wedit.set_cursor_position(Point(correction.line,
					    correction.offset));
	    wedit.set_primary_mark();
	    for (int i = 0; i < correction.incorrect.len(); i++)
		wedit.move_forward_char();

	    menu_result = splwnd.exec_correction_menu(correction);

	    if (menu_result == splChoice) {
		replace_with = correction.suggestions[
					    splwnd.get_suggestion_choice()];
	    } else if (menu_result == splEdit) {
		bool alt_kbd = wedit.get_alt_kbd();
		replace_with = dialog.query(_("Replace with:"),
			correction.incorrect, SPELER_REPLACE_HISTORY,
			InputLine::cmpltOff, &alt_kbd);
		wedit.set_alt_kbd(alt_kbd);
	    }
	}

	switch (menu_result) {
	case splAbort:
	    restore_cursor = false;
	    return false;
	    break;
	case splAbortRestoreCursor:
	    restore_cursor = true;
	    return false;
	    break;
	case splIgnore:
	    if (splwnd.is_global_decision())
		replace_table[correction.incorrect] = unistring();
	    break;
	case splAdd:
	    add_to_dictionary(correction);
	    break;

	case splChoice:
	case splEdit:
	    if (!replace_with.empty()) {
		wedit.set_cursor_position(Point(correction.line,
						correction.offset));
		wedit.replace_text(replace_with, correction.incorrect.len());
		if (splwnd.is_global_decision())
		    replace_table[correction.incorrect] = replace_with;
		// Since we modified the text, the offsets of the
		// following Correction objects must be adjusted.
		for (int i = cur_crctn + 1; i < corrections.size(); i++) {
		    if (corrections[i]->offset > correction.offset) {
			corrections[i]->offset +=
			    replace_with.len() - correction.incorrect.len();
		    }
		}
	    }
	    break;
	}

	app.update_terminal();
    }
    return true;
}

// adjust_word_offset() - the speller reports the offsets of incorrect
// words, but some spellers (like hspell) report incorrect offsets, so
// we need to detect these cases and find the words ourselves.

void adjust_word_offset(Correction &c, const unistring &str)
{
    if (str.index(c.incorrect, c.offset) != c.offset) {
	// first, search the word near the reported offset
	int from = c.offset - 10;
	c.offset = str.index(c.incorrect, (from < 0) ? 0 : from);
	if (c.offset == -1) {
	    // wasn't found, so search starting from the beginning
	    // of the paragraph.
	    if ((c.offset = str.index(c.incorrect, 0)) == -1)
		c.offset = 0;
	}
    }
}

// get_word_boundaries() - get the boundaries of the word on which the
// cursor stands.

void get_word_boundaries(const unistring &str, int cursor, int &wbeg, int &wend)
{
    // If the cursor stands just past the word, treat it as if it
    // stants on the word.
    if ((cursor == str.len() || !BiDi::is_wordch(str[cursor]))
	    && cursor > 0 && BiDi::is_wordch(str[cursor-1]))
	cursor--;
    
    wbeg = wend = cursor;
	
    if (cursor < str.len() && BiDi::is_wordch(str[cursor])) {
	while (wbeg > 0 && BiDi::is_wordch(str[wbeg-1]))
	    wbeg--;
	while (wend < str.len()-1 && BiDi::is_wordch(str[wend+1]))
	    wend++;
	wend++;
    }
}

// erase_special_characters_words() -  erases/modifies characters
// or words that may cause problems to the speller:
// 
// 0. If we're checking emails and the line is quoted (">"), erase it.
// 1. remove words with combining characters (e.g. Hebrew points)
// 2. remove ispell's "\"
// 3. convert Hebrew maqaf to ASCII one.

void erase_special_characters_words(unistring &str, bool erase_quotes)
{
    if (erase_quotes) {
	// If we're checking emails, erase lines starting
	// with ">" (with optional preceding spaces).
	int i = 0;
	while (i < str.len() && str[i] == ' ')
	    i++;
	if (i < str.len() && str[i] == '>') {
	    for (i = 0; i < str.len(); i++)
		str[i] = ' ';
	}
    }
    for (int i = 0; i < str.len(); i++) {
	if (str[i] == UNI_HEB_MAQAF)
	    str[i] = '-';
	if (str[i] == '\\') // ispell's line continuation char.
	    str[i] = ' ';
    }
    for (int i = 0; i < str.len(); i++) {
	if (mk_wcwidth(str[i]) == 0) {
	    if (BiDi::is_nsm(str[i])) {
		// delete the word in which the NSM is.
		int wbeg, wend;
		get_word_boundaries(str, i, wbeg, wend);
		for (int j = wbeg; j < wend; j++)
		    str[j] = ' ';
	    } else {
		// probably some formatting code (RLM, LRM, etc)
		str[i] = ' ';
	    }
	}
    }   
}

// erase_before_after_word() - erases the text segment preceding or the
// text segment following the word on which the cursor stands. 
 
void erase_before_after_word(unistring &str, int cursor, bool bef, bool aft)
{
    int wbeg, wend;
    get_word_boundaries(str, cursor, wbeg, wend);
    if (bef)
	for (int i = 0; i < wbeg; i++)
	    str[i] = ' ';
    if (aft) {
	// but don't erase the hebrew maqaf (ascii-transliterated)
	if (wend < str.len() && str[wend] == '-')
	    wend++;
	for (int i = wend; i < str.len(); i++)
	    str[i] = ' ';
    }
}

// spell_check() - the principal method. 

void Speller::spell_check(splRng range, EditBox &wedit, SpellerWnd &splwnd)
{
    if (!is_loaded()) {
	dialog.show_message(_("Speller is not loaded"));
	return;
    }

    bool cancel_spelling = false;

    if (range == splRngWord)
	write_line("%\n"); // exit terse mode
    else
	write_line("!\n"); // enter terse mode
    
    // Find the start and end paragraphs corresponding to
    // the requested range.
    int start_para, end_para;
    Point cursor_origin;
    wedit.get_cursor_position(cursor_origin);
    if (range == splRngAll) {
	start_para = 0;
	end_para   = wedit.get_number_of_paragraphs() - 1;
    } else {
	start_para = cursor_origin.para;
	if (range == splRngForward)
	    end_para = wedit.get_number_of_paragraphs() - 1;
	else
	    end_para = start_para;
    }
    
    // Some variabls that are used when range==splRngWord
    bool      sole_word_correct = false;
    unistring sole_word;
    unistring sole_word_root;

    bool restore_cursor = true;

    for (int i = start_para; i <= end_para && !cancel_spelling; i++)
    {
	dialog.show_message_fmt(_("Spell checking... %d/%d"),
				  i+1, wedit.get_number_of_paragraphs());
	dialog.immediate_update();
	
	unistring para = wedit.get_paragraph_text(i);

	// erase/modify some characters/words
	erase_special_characters_words(para,
		(wedit.get_syn_hlt() == EditBox::synhltEmail) && (range != splRngWord));

	if (i == start_para) {
	    if (range != splRngAll) {
		// erase text we're not supposed to check.
		erase_before_after_word(para, cursor_origin.pos,
			true, range != splRngForward);

		// after finishing checking splRgnForward/splRgnWord,
		// we restore the cursor to the start of the word on
		// which it stood.
		int wbeg, wend;
		get_word_boundaries(para, cursor_origin.pos, wbeg, wend);
		cursor_origin.pos = wbeg;

		// also, when checking a sole word, keep it because
		// we need to display it later in the dialog-line.
		if (range == splRngWord)
		    sole_word = para.substr(wbeg, wend - wbeg);
	    } else {
		// after finishing checking the whole document, we
		// restore cursor position to the first column of
		// the paragraph.
		cursor_origin.pos = 0;
	    }
	}
    
	// Convert the text to the speller encoding
	// :TODO: special treatment for UTF-8.
	cstring cstr;
	convert_from_unistr(cstr, para, conv_to_speller);

	// Send "^text" to speller
	cstr.insert(0, "^");
	cstr += "\n";
	write_line(cstr.c_str());
	
	// Read the speller reply, till encountering the empty string,
	// and construct a Corrections collection.
	Corrections corrections;
	Correction *last_corretion = NULL;
	do {
	    cstr = read_line();
	    if (cstr.size() != 0) {
		unistring ustr;
		convert_to_unistr(ustr, cstr, conv_from_speller);
		Correction *c = new Correction(u8string(ustr).c_str(), i);
		if (c->is_valid()) {
		    // store the speller-encoded word too, in case
		    // we need to feed it back (like in the "*<<word>>"
		    // command).
		    convert_from_unistr(c->incorrect_original, c->incorrect,
					conv_to_speller);
		    adjust_word_offset(*c, para);
		    corrections.add(c);
		    last_corretion = c;
		} else {
		    delete c;
    
		    // Special support for hspell's hints.
		    if ((ustr[0] == ' ' || ustr[0] == 'H') && last_corretion)
			last_corretion->add_hint(ustr.substr(1));

		    // When spell-checking a sole word, we're in
		    // non-terse mode.
		    if (range == splRngWord) {
			if (ustr[0] == '*' || ustr[0] == '+') {
			    sole_word_correct = true;
			    if (ustr[0] == '+' && ustr.len() > 2)
				sole_word_root = ustr.substr(2);
			}
		    }
		}
	    }
	} while (cstr.size() != 0);

	corrections.sort();

	// :TODO: adjust UTF-8 offsets.

	if ((cancel_spelling = terminal::was_ctrl_c_pressed()))
	    restore_cursor = false;

	// hand the Corrections collection to the method that interacts
	// with the user.
	if (!cancel_spelling && !corrections.empty()) {
	    dialog.show_message_fmt(_("A misspelling was found at %d/%d"),
				    i+1, wedit.get_number_of_paragraphs());
	    cancel_spelling = !interactive_correct(corrections,
				    wedit, splwnd, restore_cursor);
	}
    }

    wedit.unset_primary_mark();

    if (restore_cursor && range != splRngWord)
	wedit.set_cursor_position(cursor_origin);

    if (sole_word_correct) {
	if (sole_word_root.empty())
	    dialog.show_message_fmt(_("Word '%s' is correct"),
				    u8string(sole_word).c_str());
	else
	    dialog.show_message_fmt(_("Word '%s' is correct because of %s"),
				    u8string(sole_word).c_str(),
				    u8string(sole_word_root).c_str());
    } else {
	dialog.show_message(_("Spell cheking done"));
    }
}

// read_line() - read a line from the speller

cstring Speller::read_line()
{
    u8string str;
    char ch;
    while (read(fd_from_spl[0], &ch, 1)) {
	if (ch != '\n')
	    str += ch;
	else
	    break;
    }
    return str;
}

// write_line() - write a line to the speller

void Speller::write_line(const char *s)
{
    write(fd_to_spl[1], s, strlen(s));
}

