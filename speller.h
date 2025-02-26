#ifndef BDE_SPELLER_H
#define BDE_SPELLER_H

#include "editbox.h"
#include "label.h"

class Editor;

class Correction;
class Corrections;

// A SpellerWnd object represents the GUI window in which the speller results
// (the list of suggestions) are displayed, like a menu. Its public interface
// allows the Speller object to post a menu and get the user's action.

enum MenuResult {
    splAbort, splAbortRestoreCursor,
    splIgnore, splAdd, splEdit, splChoice
};

class SpellerWnd : public Widget {

    Label   label;
    EditBox editbox;
    Editor  &app;   // for update_terminal()

    bool finished;  // the modal menu executes as long as this
		    // flag is 'true'.
    void end_menu(MenuResult);

    Correction *correction;
    unistring   word_keys;
    
    MenuResult	menu_result;
    bool	global_decision;
    int		suggestion_choice;

    void clear();
    void append(const char *s);
    void append(const unistring &us);

public:

    SpellerWnd(Editor &aApp);

    MenuResult exec_correction_menu(Correction &correction);
    bool is_global_decision()    { return global_decision; }
    int  get_suggestion_choice() { return suggestion_choice; }

    HAS_ACTIONS_MAP(SpellerWnd, Dispatcher);
    HAS_BINDINGS_MAP(SpellerWnd, Dispatcher);

    INTERACTIVE void ignore_word();
    INTERACTIVE void edit_replacement();
    INTERACTIVE void add_to_dict();
    INTERACTIVE void set_global_decision();
    INTERACTIVE void abort_spelling();
    INTERACTIVE void abort_spelling_restore_cursor();

    INTERACTIVE void layout_windows();
    INTERACTIVE void refresh();

    virtual bool handle_event(const Event &evt);
    virtual void update();
    virtual bool is_dirty() const;
    virtual void invalidate_view();
    virtual void resize(int lines, int columns, int y, int x);
    void update_cursor() { editbox.update_cursor(); }
};

// A Speller object does the communication with the speller. It gets
// the incorrect words and uses the SpellerWnd object to provide
// the user with a menu.

class DialogLine;
class Converter;

class Speller {

    // pipes for communication with the speller process.
    int fd_to_spl[2];
    int fd_from_spl[2];
    
    Editor &app; // for update_terminal()
    DialogLine &dialog;
    bool loaded;
    Converter *conv_to_speller, *conv_from_speller;

    cstring read_line();
    void    write_line(const char *s);

    void add_to_dictionary(Correction &correction);

    bool interactive_correct(Corrections &corrections,
			     EditBox &wedit,
			     SpellerWnd &splwnd,
			     bool &restore_cursor);
public:

    enum splRng { splRngAll, splRngForward, splRngWord };

    Speller(Editor &app, DialogLine &aDialog);
    
    bool is_loaded() const { return loaded; }
    
    bool    load(const char *cmd, const char *encoding);
    void    unload();

    void spell_check(splRng range,
		     EditBox &wedit,
		     SpellerWnd &splwnd);
};

void UNLOAD_SPELLER();

#endif

