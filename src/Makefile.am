ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

AM_CPPFLAGS = 						\
		-Wall -Werror 				\
		$(BOOKS_CFLAGS) 			\
		-DDATADIR=\""$(datadir)"\"

bin_PROGRAMS = books

BUILT_SOURCES = 			\
		books-resources.c

books_SOURCES = 			\
		main.c 				\
		books-collection.c 	\
		books-collection.h 	\
		books-epub.c 		\
		books-epub.h 		\
		books-window.c 		\
		books-window.h 		\
		books-main-window.c \
		books-main-window.h \
		books.gresource.xml \
		ui/books.ui 		\
		$(BUILT_SOURCES)

books_LDADD = $(BOOKS_LIBS)

RESOURCES = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies books.gresource.xml)

books-resources.c: books.gresource.xml $(RESOURCES)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --generate --c-name books books.gresource.xml

EXTRA_DIST = 				\
		books.gresource.xml

CLEANFILES = *~
