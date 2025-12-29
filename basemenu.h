#ifndef BDE_BASEMENU_H
#define BDE_BASEMENU_H

#include "widget.h"
#include <cstring>

struct MenuItem {
    const char *action;
    const char *label;
    int         state_id;
    MenuItem   *submenu;
    unsigned long command_parameter1;
    unsigned long command_parameter2;
    const char   *command_parameter3;
    const char *desc;
    Event       evt;
    unichar     shortkey;
};

typedef MenuItem PulldownMenu[];

struct MenubarItem {
    const char *label;
    MenuItem   *submenu;
};

typedef MenubarItem MenubarMenu[];

class PopupMenu : public Widget {

public:
    // The result of the user interaction:
    enum PostResult { mnuPrev, mnuNext, mnuSelect, mnuCancel };

protected:

    MenuItem *mnu;  // The menu.
    int count;	    // Number of menu items.
    int top;	    // If the menu is too long to fit the screen,
		    // 'top' points to the first visible item.
    int current;    // The highlightd item.
    PostResult post_result;
    PopupMenu *parent;

private:

    void draw_frame();
    int  get_optimal_width();
    int  get_item_optimal_width(int item);
    void update_ancestors();
    void complete_menu(PulldownMenu mnu);
    void reposition(int x, int y);
    void end_modal(PostResult rslt);

protected:

    virtual void show_hint(const char *hint) = 0;
    virtual void clear_other_popups() = 0;
    virtual bool get_item_state(int id) = 0;
    virtual Dispatcher *get_primary_target() = 0;
    virtual Dispatcher *get_secondary_target() = 0;
    virtual PopupMenu *create_popupmenu(PopupMenu *aParent, PulldownMenu mnu) = 0;
    virtual bool handle_event(const Event &evt);
    virtual void do_command(unsigned long parameter1, unsigned long parameter2,
			    const char *parameter3) = 0;

public:

    HAS_ACTIONS_MAP(PopupMenu, Dispatcher);
    HAS_BINDINGS_MAP(PopupMenu, Dispatcher);
    
    PopupMenu(PopupMenu *aParent, PulldownMenu aMnu);
    void init(PulldownMenu mnu);
    PostResult post(int x, int y, Event &evt);

    INTERACTIVE void prev_menu();
    INTERACTIVE void next_menu();
    INTERACTIVE void select();
    INTERACTIVE void cancel_menu();
    INTERACTIVE void screen_resize();

    INTERACTIVE void move_previous_item();
    INTERACTIVE void move_next_item();
    INTERACTIVE void move_first_item();
    INTERACTIVE void move_last_item();
    
    // from base Widget class:
    virtual bool is_dirty() const { return true; }
    virtual void invalidate_view() {}
    virtual void update();
};

class Menubar : public Widget {

protected:

    MenubarItem *mnu;
    int count;
    int current;
    bool dirty;

    void set_current(int i);
    int get_ofs(int item);
    virtual void refresh_screen() = 0;
    virtual PopupMenu *create_popupmenu(PulldownMenu mnu) = 0;
    virtual bool handle_event(const Event &evt);
    
public:

    HAS_ACTIONS_MAP(Menubar, Dispatcher);
    HAS_BINDINGS_MAP(Menubar, Dispatcher);

    Menubar();
    void init(MenubarItem *aMnu);
    void exec();

    INTERACTIVE void select();
    INTERACTIVE void next_menu();
    INTERACTIVE void prev_menu();
    INTERACTIVE void screen_resize();

    // from base Widget class:
    virtual bool is_dirty() const { return dirty; }
    virtual void invalidate_view() { dirty = true; }
    virtual void update();
    virtual void resize(int lines, int columns, int y, int x);
};

#endif

