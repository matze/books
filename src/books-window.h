#ifndef BOOKS_WINDOW_H
#define BOOKS_WINDOW_H

#include <gtk/gtk.h>

#include "books-epub.h"

G_BEGIN_DECLS

#define BOOKS_TYPE_WINDOW             (books_window_get_type())
#define BOOKS_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), BOOKS_TYPE_WINDOW, BooksWindow))
#define BOOKS_IS_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOOKS_TYPE_WINDOW))
#define BOOKS_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), BOOKS_TYPE_WINDOW, BooksWindowClass))
#define BOOKS_IS_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), BOOKS_TYPE_WINDOW))
#define BOOKS_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), BOOKS_TYPE_WINDOW, BooksWindowClass))


typedef struct _BooksWindow           BooksWindow;
typedef struct _BooksWindowClass      BooksWindowClass;
typedef struct _BooksWindowPrivate    BooksWindowPrivate;

struct _BooksWindow {
    GtkWindow parent;

    BooksWindowPrivate *priv;
};

struct _BooksWindowClass {
    GtkWindowClass parent_class;
};

GtkWidget * books_window_new          (void);
void        books_window_set_epub     (BooksWindow *window,
                                       BooksEpub *epub);
GType       books_window_get_type     (void);

G_END_DECLS

#endif
