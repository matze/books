#ifndef BOOKS_REMOVED_DIALOG_H
#define BOOKS_REMOVED_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BOOKS_TYPE_REMOVED_DIALOG             (books_removed_dialog_get_type())
#define BOOKS_REMOVED_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), BOOKS_TYPE_REMOVED_DIALOG, BooksRemovedDialog))
#define BOOKS_IS_REMOVED_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOOKS_TYPE_REMOVED_DIALOG))
#define BOOKS_REMOVED_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), BOOKS_TYPE_REMOVED_DIALOG, BooksRemovedDialogClass))
#define BOOKS_IS_REMOVED_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), BOOKS_TYPE_REMOVED_DIALOG))
#define BOOKS_REMOVED_DIALOG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), BOOKS_TYPE_REMOVED_DIALOG, BooksRemovedDialogClass))


typedef struct _BooksRemovedDialog           BooksRemovedDialog;
typedef struct _BooksRemovedDialogClass      BooksRemovedDialogClass;
typedef struct _BooksRemovedDialogPrivate    BooksRemovedDialogPrivate;

struct _BooksRemovedDialog {
    GtkDialog parent;

    BooksRemovedDialogPrivate *priv;
};

struct _BooksRemovedDialogClass {
    GtkDialogClass parent_class;
};

GtkDialog   *books_removed_dialog_new       (GtkTreeModel *model);
GType        books_removed_dialog_get_type  (void);

G_END_DECLS

#endif
