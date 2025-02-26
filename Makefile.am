AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = geresh
geresh_SOURCES = \
	bidi.cc bidi.h \
	basemenu.cc basemenu.h \
	converters.cc converters.h \
	dbg.cc dbg.h \
	dialogline.cc dialogline.h \
	editbox.cc editbox2.cc editbox.h \
	editor.cc editor.h \
	event.cc event.h \
	helpbox.cc helpbox.h \
	inputline.cc inputline.h \
	io.cc io.h \
	iso88598.cc iso88598.h \
	label.cc label.h \
	main.cc bindings.cc \
	menus.cc menus.h \
	mk_wcwidth.cc mk_wcwidth.h \
	question.cc question.h \
	scrollbar.cc scrollbar.h \
	speller.cc speller.h \
	shaping.cc shaping.h \
	statusline.cc statusline.h \
	terminal.cc terminal.h \
	themes.cc themes.h \
	transtbl.cc transtbl.h \
	types.cc types.h \
	undo.cc undo.h \
	utf8.cc utf8.h \
	widget.cc widget.h \
	directvect.h dispatcher.h my_wctob.h \
	pathnames.h point.h univalues.h

bin_SCRIPTS = pgeresh

THEME_DIR = themes
THEMES = \
	$(THEME_DIR)/default.thm \
	$(THEME_DIR)/default_bw.thm \
	$(THEME_DIR)/mc.thm \
	$(THEME_DIR)/green.thm \
	$(THEME_DIR)/borland.thm \
	$(THEME_DIR)/occult.thm \
	$(THEME_DIR)/arnold.thm \
	$(THEME_DIR)/README.themes

pkgdata_DATA = \
	MANUAL.he kbdtab reprtab transtab

EXTRA_DIST = \
	MANUAL.he kbdtab reprtab transtab \
	$(THEMES)

localedir = $(datadir)/locale
INCLUDES = -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\"

# Hmmm.... what's the correct way to create a directory
# under pkgdatadir? How can I install the themes using only 
# pkgdata_DATA? Meanwhile, I'll use the following hack:

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/$(THEME_DIR)
	@list='$(THEMES)'; for p in $$list; do \
	  if test -f $(srcdir)/$$p; then \
	    echo " $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(pkgdatadir)/$$p"; \
	    $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(pkgdatadir)/$$p; \
	  else if test -f $$p; then \
	    echo " $(INSTALL_DATA) $$p $(DESTDIR)$(pkgdatadir)/$$p"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(pkgdatadir)/$$p; \
	  fi; fi; \
	done

uninstall-local:
	list='$(THEMES)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(pkgdatadir)/$$p; \
	done
