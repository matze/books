ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CPPFLAGS = $(BOOKS_CFLAGS)

bin_PROGRAMS = books

BUILT_SOURCES = 			\
		books-resources.c

books_SOURCES = 			\
		main.c 				\
		books-epub.c 		\
		books-window.c 		\
		$(BUILT_SOURCES)

books_LDADD = $(BOOKS_LIBS)

RESOURCES = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies books.gresource.xml)

books-resources.c: books.gresource.xml $(RESOURCES)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --generate --c-name books books.gresource.xml

EXTRA_DIST =
		books.gresource.xml
