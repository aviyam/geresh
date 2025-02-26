#ifndef BDE_HELPBOX_H
#define BDE_HELPBOX_H

#include "editbox.h"
#include "label.h"

class Editor;

class HelpBox : public EditBox {

    Editor *app;
    Label statusmsg;

    int toc_first_line;
    int toc_last_line;
    std::vector<unistring> toc_items;
    
    struct Position {
	CombinedLine top_line;
	Point cursor;
    };
    static std::vector<Position> positions_stack;

public:

    HAS_ACTIONS_MAP(HelpBox, EditBox);
    HAS_BINDINGS_MAP(HelpBox, EditBox);

    HelpBox(Editor *aApp, const EditBox &settings);
    bool load_user_manual();
    void jump_to_topic(const char *topic);

    void exec();
    virtual bool handle_event(const Event &evt);

    INTERACTIVE void layout_windows();
    INTERACTIVE void refresh_and_center();
    void push_position();
    INTERACTIVE void pop_position();
    INTERACTIVE void move_to_toc();
    INTERACTIVE void jump_to_topic();

protected:

    void scan_toc();
    virtual void do_syntax_highlight(const unistring &str, 
	    AttributeArray &attributes, int para_num);
};

#endif

