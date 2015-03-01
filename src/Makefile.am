NULL =

bin_PROGRAMS = gnome-dictionary

BUILT_SOURCES = gdict-resources.c

gnome_dictionary_CPPFLAGS = \
	-DPREFIX=\""$(prefix)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DPKGDATADIR=\""$(datadir)/gnome-dictionary"\" \
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(NULL)

gnome_dictionary_CFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libgdict \
	-I$(top_builddir)/libgdict \
	$(GDICT_CFLAGS) \
	$(NULL)

gnome_dictionary_SOURCES = \
	gdict-about.c		\
	gdict-about.h		\
	gdict-app.c		\
	gdict-app.h		\
	gdict-common.c 		\
	gdict-common.h		\
	gdict-pref-dialog.c	\
	gdict-pref-dialog.h	\
	gdict-print.c 		\
	gdict-print.h 		\
	gdict-sidebar.c		\
	gdict-sidebar.h		\
	gdict-source-dialog.c 	\
	gdict-source-dialog.h 	\
	gdict-window.c 		\
	gdict-window.h 		\
	main.c			\
	$(NULL)

nodist_gnome_dictionary_SOURCES = gdict-resources.c

gnome_dictionary_LDADD = $(top_builddir)/libgdict/libgdict-1.0.la $(GDICT_LIBS) $(LIBM)
gnome_dictionary_DEPENDENCIES = $(top_builddir)/libgdict/libgdict-1.0.la

CLEANFILES = gdict-resources.c
EXTRA_DIST = gdict.gresource.xml $(resource_files)

resource_files = $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gdict.gresource.xml)

gdict-resources.c: gdict.gresource.xml $(resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $<

-include $(top_srcdir)/git.mk
