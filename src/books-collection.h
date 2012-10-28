#ifndef BOOKS_COLLECTION_H
#define BOOKS_COLLECTION_H

#include <gtk/gtk.h>
#include <books-epub.h>

G_BEGIN_DECLS

#define BOOKS_TYPE_COLLECTION             (books_collection_get_type())
#define BOOKS_COLLECTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), BOOKS_TYPE_COLLECTION, BooksCollection))
#define BOOKS_IS_COLLECTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOOKS_TYPE_COLLECTION))
#define BOOKS_COLLECTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), BOOKS_TYPE_COLLECTION, BooksCollectionClass))
#define BOOKS_IS_COLLECTION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), BOOKS_TYPE_COLLECTION))
#define BOOKS_COLLECTION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), BOOKS_TYPE_COLLECTION, BooksCollectionClass))


typedef struct _BooksCollection           BooksCollection;
typedef struct _BooksCollectionClass      BooksCollectionClass;
typedef struct _BooksCollectionPrivate    BooksCollectionPrivate;

struct _BooksCollection {
    GObject parent;

    BooksCollectionPrivate *priv;
};

struct _BooksCollectionClass {
    GObjectClass parent_class;
};

enum {
    BOOKS_COLLECTION_AUTHOR_COLUMN,
    BOOKS_COLLECTION_TITLE_COLUMN,
    BOOKS_COLLECTION_MARKUP_COLUMN,
    BOOKS_COLLECTION_PATH_COLUMN,
    BOOKS_COLLECTION_ICON_COLUMN,
    BOOKS_COLLECTION_N_COLUMNS
};

BooksCollection *books_collection_new           (void);
GtkTreeModel    *books_collection_get_model     (BooksCollection    *collection);
void             books_collection_add_book      (BooksCollection    *collection,
                                                 BooksEpub          *epub,
                                                 const gchar        *path);
void             books_collection_remove_book   (BooksCollection    *collection,
                                                 GtkTreeIter        *iter);
BooksEpub       *books_collection_get_book      (BooksCollection    *collection,
                                                 GtkTreePath        *path,
                                                 GError            **error);
GType            books_collection_get_type      (void);

G_END_DECLS

#endif
