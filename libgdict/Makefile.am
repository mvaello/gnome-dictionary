NULL =

AM_CPPFLAGS = \
	-I$(top_srcdir)/libgdict				\
	-I$(top_builddir)/libgdict				\
	-DG_LOG_DOMAIN=\"Gdict\" 				\
	-DDATADIR=\""$(datadir)"\" 				\
	-DLIBDIR=\""$(libdir)"\" 				\
	-DSYSCONFDIR=\""$(sysconfdir)"\" 			\
	-DPREFIX=\""$(prefix)"\" 				\
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"  \
	-DGDICTSOURCESDIR=\""$(datadir)/gdict-1.0/sources"\" 	\
	-DGDICT_ENABLE_INTERNALS=1 				\
	-DG_DISABLE_DEPRECATED					\
	-DGDK_DISABLE_DEPRECATED				\
	-UGTK_DISABLE_DEPRECATED				\
	-DPANGO_DISABLE_DEPRECATED				\
	-DG_DISABLE_SINGLE_INCLUDES				\
	-DGTK_DISABLE_SINGLE_INCLUDES				\
	$(NULL)

sources_h = \
	gdict-context.h			\
	gdict-client-context.h		\
	gdict-database-chooser.h 	\
	gdict-defbox.h 			\
	gdict-source-chooser.h		\
	gdict-source-loader.h		\
	gdict-source.h			\
	gdict-speller.h			\
	gdict-strategy-chooser.h 	\
	gdict-utils.h			\
	$(NULL)

sources_h_priv = \
	gdict-context-private.h		\
	gdict-debug.h			\
	gdict-private.h			\
	$(NULL)

sources_c = \
	gdict-context.c			\
	gdict-client-context.c		\
	gdict-database-chooser.c	\
	gdict-defbox.c			\
	gdict-source-chooser.c		\
	gdict-source-loader.c		\
	gdict-source.c			\
	gdict-speller.c			\
	gdict-strategy-chooser.c	\
	gdict-utils.c			\
	$(NULL)

BUILT_SOURCES = \
	gdict-enum-types.h	\
	gdict-enum-types.c	\
	gdict-marshal.h		\
	gdict-marshal.c

lib_LTLIBRARIES = libgdict-1.0.la

# GNOME Dictionary shared library
libgdict_1_0_la_SOURCES = $(BUILT_SOURCES) $(sources_h)	$(sources_h_priv) $(sources_c)
libgdict_1_0_la_CFLAGS = $(GDICT_CFLAGS) $(GDICT_DEBUG_CFLAGS) $(MAINTAINER_CFLAGS)
libgdict_1_0_la_LIBADD = $(GDICT_LIBS)
libgdict_1_0_la_LDFLAGS = -version-info $(LIBGDICT_LT_VERSION)	-export-dynamic -no-undefined

libgdict_includedir = $(includedir)/gdict-1.0/gdict
libgdict_include_HEADERS = 	\
	$(sources_h)		\
	$(srcdir)/gdict.h	\
	$(NULL)

nodist_libgdict_include_HEADERS = \
	gdict-enum-types.h	\
	gdict-version.h		\
	$(NULL)

stamp_files = stamp-gdict-enum-types.h stamp-gdict-marshal.h

CLEANFILES = $(stamp_files) $(BUILT_SOURCES)
DISTCLEANFILES = gdict-version.h
MAINTAINERCLEANFILES = $(stamp_files) $(gdict_built_files) gdict-version.h

EXTRA_DIST = 			\
	gdict-version.h.in 	\
	gdict-enum-types.h.in	\
	gdict-enum-types.c.in	\
	gdict-marshal.list	\
	gdict-1.0.pc.in		\
	$(NULL)

# Introspection
include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all

if HAVE_INTROSPECTION
INTROSPECTION_GIRS += Gdict-1.0.gir

Gdict-1.0.gir: libgdict-1.0.la
Gdict_1_0_gir_NAMESPACE = Gdict
Gdict_1_0_gir_VERSION = 1.0
Gdict_1_0_gir_INCLUDES = Gtk-3.0
Gdict_1_0_gir_CFLAGS = $(AM_CPPFLAGS)
Gdict_1_0_gir_LIBS = libgdict-1.0.la
Gdict_1_0_gir_FILES = $(libgdict_1_0_la_SOURCES)

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

gdict-marshal.h: stamp-gdict-marshal.h
	@true
stamp-gdict-marshal.h: gdict-marshal.list Makefile
	$(AM_V_GEN)$(GLIB_GENMARSHAL) \
	  $(srcdir)/gdict-marshal.list --header --prefix=gdict_marshal >> xgen-gmh \
	&& ( cmp -s xgen-gmh gdict-marshal.h || cp xgen-gmh gdict-marshal.h ) \
	&& rm -f xgen-gmh \
	&& echo timestamp > $@

gdict-marshal.c: gdict-marshal.list Makefile
	$(AM_V_GEN)( echo "#include \"gdict-marshal.h\""; echo; \
	  $(GLIB_GENMARSHAL) \
	  $(srcdir)/gdict-marshal.list --body --prefix=gdict_marshal ) >> xgen-gmc \
	&& cp xgen-gmc gdict-marshal.c \
	&& rm -f xgen-gmc

gdict-enum-types.h: stamp-gdict-enum-types.h
	@true
stamp-gdict-enum-types.h: $(sources_h) gdict-enum-types.h.in Makefile
	$(AM_V_GEN) $(GLIB_MKENUMS) --template $(srcdir)/gdict-enum-types.h.in \
	  $(addprefix $(srcdir)/,$(sources_h)) >> xgen-ceth \
	&& ( cmp -s xgen-ceth gdict-enum-types.h || cp xgen-ceth gdict-enum-types.h ) \
	&& rm -f xgen-ceth \
	&& echo timestamp > $@

gdict-enum-types.c: gdict-enum-types.h gdict-enum-types.c.in Makefile
	$(AM_V_GEN) $(GLIB_MKENUMS) --template $(srcdir)/gdict-enum-types.c.in \
	  $(addprefix $(srcdir)/,$(sources_h)) >> xgen-cetc \
	&& cp xgen-cetc gdict-enum-types.c \
	&& rm -f xgen-cetc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gdict-1.0.pc

distclean-local:
	if test $(srcdir) != .; then \
		rm -f $(MAINTAINERCLEANFILES); \
	fi

-include $(top_srcdir)/git.mk
