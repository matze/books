#ifndef BOOKS_EPUB_H
#define BOOKS_EPUB_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BOOKS_TYPE_EPUB             (books_epub_get_type())
#define BOOKS_EPUB(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), BOOKS_TYPE_EPUB, BooksEpub))
#define BOOKS_IS_EPUB(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), BOOKS_TYPE_EPUB))
#define BOOKS_EPUB_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), BOOKS_TYPE_EPUB, BooksEpubClass))
#define BOOKS_IS_EPUB_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), BOOKS_TYPE_EPUB))
#define BOOKS_EPUB_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), BOOKS_TYPE_EPUB, BooksEpubClass))

#define BOOKS_EPUB_ERROR books_epub_error_quark()

typedef enum {
    BOOKS_EPUB_ERROR_INVALID_ARCHIVE_FORMAT,
    BOOKS_EPUB_ERROR_NO_META_DATA
} BooksEpubError;

typedef struct _BooksEpub           BooksEpub;
typedef struct _BooksEpubClass      BooksEpubClass;
typedef struct _BooksEpubPrivate    BooksEpubPrivate;

struct _BooksEpub {
    GObject parent_instance;

    BooksEpubPrivate *priv;
};

struct _BooksEpubClass {
    GObjectClass parent_class;
};

BooksEpub  *books_epub_new          (void);
gboolean    books_epub_open         (BooksEpub      *epub,
                                     const gchar   *filename,
                                     GError       **error);
gchar      *books_epub_get_content  (BooksEpub      *epub);
void        books_epub_next         (BooksEpub      *epub);
void        books_epub_previous     (BooksEpub      *epub);
gboolean    books_epub_is_first     (BooksEpub      *epub);
gboolean    books_epub_is_last      (BooksEpub      *epub);
GType       books_epub_get_type     (void);
GQuark      books_epub_error_quark  (void);

G_END_DECLS

#endif
