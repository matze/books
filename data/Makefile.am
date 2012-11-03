desktop_in_files = books.desktop.in.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

gsettings_ENUM_NAMESPACE = com.github.matze.books
gsettings_ENUM_FILES = $(top_srcdir)/src/*.h
gsettings_SCHEMAS = com.github.matze.books.gschema.xml

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

cssdir = $(datadir)/books
css_DATA = books.css

EXTRA_DIST = 										\
		$(desktop_in_files) 						\
		$(css_DATA) 								\
		com.github.matze.books.gschema.xml.in.in

CLEANFILES = 					\
		$(desktop_DATA) 		\
		$(gsettings_SCHEMAS)

DISTCLEANFILES = 				\
		$(desktop_DATA) 		\
		$(gsettings_SCHEMAS)
