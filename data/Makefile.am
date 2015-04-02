NULL =

EXTRA_DIST =
CLEANFILES =

man_MANS = gnome-dictionary.1
EXTRA_DIST += $(man_MANS)

gdictappdir = $(datadir)/applications
gdictapp_in_files = org.gnome.Dictionary.desktop.in
gdictapp_DATA = $(gdictapp_in_files:.desktop.in=.desktop)
EXTRA_DIST += org.gnome.Dictionary.desktop.in.in
CLEANFILES += $(gdictapp_in_files) $(gdictapp_DATA)

$(gdictapp_in_files): $(gdictapp_in_files:.desktop.in=.desktop.in.in)
	$(AM_V_GEN)sed -e "s|\@VERSION\@|@VERSION@|" $< > $@

@INTLTOOL_DESKTOP_RULE@

dbusservicedir = $(datadir)/dbus-1/services
dbusservice_DATA = org.gnome.Dictionary.service

org.gnome.Dictionary.service: Makefile
	$(AM_V_GEN) (echo '[D-BUS Service]'; \
		     echo 'Name=org.gnome.Dictionary'; \
		     echo 'Exec=${bindir}/gnome-dictionary --gapplication-service') > $@.tmp && \
		     mv $@.tmp $@

CLEANFILES += $(dbusservice_DATA)

@INTLTOOL_XML_RULE@
appdatadir = $(datadir)/appdata
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdata_in_files = org.gnome.Dictionary.appdata.xml.in
EXTRA_DIST += $(appdata_in_files)
CLEANFILES += $(appdata_DATA)

# dictionary sources definitions
dictsourcedir = $(datadir)/gdict-1.0/sources
dictsource_in_files = \
	default.desktop.in \
	spanish.desktop.in \
	thai.desktop.in \
	$(NULL)
dictsource_DATA = $(dictsource_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST += $(dictsource_in_files)
CLEANFILES += $(dictsource_DATA)

gsettings_SCHEMAS = org.gnome.dictionary.gschema.xml

@GSETTINGS_RULES@

-include $(top_srcdir)/git.mk
