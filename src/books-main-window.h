#ifndef BOOKS_MAIN_WINDOW_H
#define BOOKS_MAIN_WINDOW_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BOOKS_TYPE_MAIN_WINDOW             (books_main_window_get_type())
#define BOOKS_MAIN_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), BOOKS_TYPE_MAIN_WINDOW, BooksMainWindow))
#define BOOKS_IS_MAIN_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOOKS_TYPE_MAIN_WINDOW))
#define BOOKS_MAIN_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), BOOKS_TYPE_MAIN_WINDOW, BooksMainWindowClass))
#define BOOKS_IS_MAIN_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), BOOKS_TYPE_MAIN_WINDOW))
#define BOOKS_MAIN_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), BOOKS_TYPE_MAIN_WINDOW, BooksMainWindowClass))


typedef struct _BooksMainWindow           BooksMainWindow;
typedef struct _BooksMainWindowClass      BooksMainWindowClass;
typedef struct _BooksMainWindowPrivate    BooksMainWindowPrivate;

struct _BooksMainWindow {
    GtkWindow parent;

    BooksMainWindowPrivate *priv;
};

struct _BooksMainWindowClass {
    GtkWindowClass parent_class;
};

GtkWidget * books_main_window_new          (void);
GType       books_main_window_get_type     (void);

G_END_DECLS

#endif
