#ifndef BOOKS_PREFERENCES_DIALOG_H
#define BOOKS_PREFERENCES_DIALOG_H

#include <gtk/gtk.h>

#include "books-main-window.h"

G_BEGIN_DECLS

#define BOOKS_TYPE_PREFERENCES_DIALOG             (books_preferences_dialog_get_type())
#define BOOKS_PREFERENCES_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), BOOKS_TYPE_PREFERENCES_DIALOG, BooksPreferencesDialog))
#define BOOKS_IS_PREFERENCES_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOOKS_TYPE_PREFERENCES_DIALOG))
#define BOOKS_PREFERENCES_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), BOOKS_TYPE_PREFERENCES_DIALOG, BooksPreferencesDialogClass))
#define BOOKS_IS_PREFERENCES_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), BOOKS_TYPE_PREFERENCES_DIALOG))
#define BOOKS_PREFERENCES_DIALOG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), BOOKS_TYPE_PREFERENCES_DIALOG, BooksPreferencesDialogClass))


typedef struct _BooksPreferencesDialog           BooksPreferencesDialog;
typedef struct _BooksPreferencesDialogClass      BooksPreferencesDialogClass;
typedef struct _BooksPreferencesDialogPrivate    BooksPreferencesDialogPrivate;

typedef enum {
    BOOKS_STYLE_SHEET_PUBLISHER,
    BOOKS_STYLE_SHEET_BOOKS,
} BooksStyleSheetPreference;

struct _BooksPreferencesDialog {
    GtkDialog parent;

    BooksPreferencesDialogPrivate *priv;
};

struct _BooksPreferencesDialogClass {
    GtkDialogClass parent_class;
};

void    books_show_preferences_dialog       (BooksMainWindow *parent);
GType   books_preferences_dialog_get_type   (void);

G_END_DECLS

#endif
