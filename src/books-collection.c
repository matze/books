#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <sqlite3.h>
#include <glib/gi18n.h>

#include "books-collection.h"


G_DEFINE_TYPE(BooksCollection, books_collection, G_TYPE_OBJECT)

#define BOOKS_COLLECTION_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), BOOKS_TYPE_COLLECTION, BooksCollectionPrivate))

enum {
    PROP_0,
    PROP_FILTER_TERM
};

struct _BooksCollectionPrivate {
    GtkListStore    *store;
    GtkTreeModel    *sorted;
    GtkTreeModel    *filtered;
    sqlite3         *db;
    gchar           *filter_term;
};

BooksCollection *
books_collection_new (void)
{
    return BOOKS_COLLECTION (g_object_new (BOOKS_TYPE_COLLECTION, NULL));
}

GtkTreeModel *
books_collection_get_model (BooksCollection *collection)
{
    g_return_val_if_fail (BOOKS_IS_COLLECTION (collection), NULL);
    return collection->priv->sorted;
}

void
books_collection_add_book (BooksCollection *collection,
                           BooksEpub *epub,
                           const gchar *path)
{
    BooksCollectionPrivate *priv;
    GtkTreeIter iter;
    const gchar *author;
    const gchar *title;
    const gchar *insert_sql = "INSERT INTO books (author, title, path) VALUES (?, ?, ?)";
    sqlite3_stmt *insert_stmt = NULL;

    g_return_if_fail (BOOKS_IS_COLLECTION (collection));

    priv = collection->priv;
    author = books_epub_get_meta (epub, "creator");
    title = books_epub_get_meta (epub, "title");

    gtk_list_store_append (priv->store, &iter);
    gtk_list_store_set (priv->store, &iter,
                        BOOKS_COLLECTION_AUTHOR_COLUMN, author,
                        BOOKS_COLLECTION_TITLE_COLUMN, title,
                        BOOKS_COLLECTION_PATH_COLUMN, path,
                        -1);

    sqlite3_prepare_v2 (priv->db, insert_sql, -1, &insert_stmt, NULL);
    sqlite3_bind_text (insert_stmt, 1, author, strlen (author), NULL);
    sqlite3_bind_text (insert_stmt, 2, title, strlen (title), NULL);
    sqlite3_bind_text (insert_stmt, 3, path, strlen (path), NULL);
    sqlite3_step (insert_stmt);
    sqlite3_finalize (insert_stmt);
}

void
books_collection_remove_book (BooksCollection *collection,
                              GtkTreeIter *iter)
{
    BooksCollectionPrivate *priv;
    GtkTreeIter filtered_iter;
    GtkTreeIter real_iter;
    gchar *path;
    const gchar *remove_sql = "DELETE FROM books WHERE path=?";
    sqlite3_stmt *remove_stmt = NULL;

    g_return_if_fail (BOOKS_IS_COLLECTION (collection));
    priv = collection->priv;

    gtk_tree_model_get (priv->sorted, iter, BOOKS_COLLECTION_PATH_COLUMN, &path, -1);
    gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (priv->sorted), &filtered_iter, iter);
    gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (priv->filtered), &real_iter, &filtered_iter);
    gtk_list_store_remove (priv->store, &real_iter);
    gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filtered));

    /*
     * TODO: sqlite operations are noticeable. We should execute them
     * asynchronously.
     */
    sqlite3_prepare_v2 (priv->db, remove_sql, -1, &remove_stmt, NULL);
    sqlite3_bind_text (remove_stmt, 1, path, strlen (path), NULL);
    sqlite3_step (remove_stmt);
    sqlite3_finalize (remove_stmt);

    g_free (path);
}

BooksEpub *
books_collection_get_book (BooksCollection *collection,
                           GtkTreePath *path,
                           GError **error)
{
    BooksCollectionPrivate *priv;
    GtkTreePath *filtered_path;
    GtkTreePath *real_path;
    GtkTreeIter iter;

    g_return_val_if_fail (BOOKS_IS_COLLECTION (collection), NULL);

    priv = collection->priv;
    filtered_path = gtk_tree_model_sort_convert_path_to_child_path (GTK_TREE_MODEL_SORT (priv->sorted),
                                                                    path);

    real_path = gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (priv->filtered),
                                                                  filtered_path);

    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->store), &iter, real_path)) {
        BooksEpub *epub;
        gchar *filename;

        gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter, BOOKS_COLLECTION_PATH_COLUMN, &filename, -1);
        epub = books_epub_new ();

        if (books_epub_open (epub, filename, error))
            return epub;
        else
            g_object_unref (epub);

        g_free (filename);
    }

    return NULL;
}

static int
insert_row_into_model (gpointer user_data,
                       gint argc,
                       gchar **argv,
                       gchar **column)
{
    GtkListStore *store;
    GtkTreeIter iter;

    g_assert (argc == 3);
    store = GTK_LIST_STORE (user_data);

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
                        BOOKS_COLLECTION_AUTHOR_COLUMN, argv[0],
                        BOOKS_COLLECTION_TITLE_COLUMN, argv[1],
                        BOOKS_COLLECTION_PATH_COLUMN, argv[2],
                        -1);
    return 0;
}

static gboolean
row_visible (GtkTreeModel *model,
             GtkTreeIter *iter,
             BooksCollectionPrivate *priv)
{
    gboolean visible;
    gchar *lowered_author;
    gchar *lowered_title;
    gchar *lowered_term;
    gchar *author;
    gchar *title;

    if (priv->filter_term == NULL)
        return TRUE;

    gtk_tree_model_get (model, iter,
                        BOOKS_COLLECTION_AUTHOR_COLUMN, &author,
                        BOOKS_COLLECTION_TITLE_COLUMN, &title,
                        -1);

    if (author == NULL || title == NULL)
        return TRUE;

    lowered_term = g_utf8_strdown (priv->filter_term, -1);
    lowered_author = g_utf8_strdown (author, -1);
    lowered_title = g_utf8_strdown (title, -1);

    visible = strstr (lowered_author, lowered_term) != NULL ||
              strstr (lowered_title, lowered_term) != NULL;

    g_free (lowered_author);
    g_free (lowered_title);
    g_free (lowered_term);
    g_free (author);
    g_free (title);

    return visible;
}

static void
books_collection_dispose (GObject *object)
{
    G_OBJECT_CLASS (books_collection_parent_class)->dispose (object);
}

static void
books_collection_finalize (GObject *object)
{
    BooksCollectionPrivate *priv;

    priv = BOOKS_COLLECTION_GET_PRIVATE (object);
    g_free (priv->filter_term);
    sqlite3_close (priv->db);

    G_OBJECT_CLASS (books_collection_parent_class)->finalize (object);
}

static void
books_collection_set_property (GObject *object,
                               guint property_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
    BooksCollectionPrivate *priv;

    priv = BOOKS_COLLECTION_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_FILTER_TERM:
            if (priv->filter_term != NULL)
                g_free (priv->filter_term);

            priv->filter_term = g_strdup (g_value_get_string (value));
            gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filtered));
            break;
            
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
books_collection_get_property(GObject *object,
                              guint property_id,
                              GValue *value,
                              GParamSpec *pspec)
{
    BooksCollectionPrivate *priv;

    priv = BOOKS_COLLECTION_GET_PRIVATE (object);

    switch (property_id) {
        case PROP_FILTER_TERM:
            g_value_set_string (value, priv->filter_term);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}


static void
books_collection_class_init (BooksCollectionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->set_property = books_collection_set_property;
    object_class->get_property = books_collection_get_property;
    object_class->dispose = books_collection_dispose;
    object_class->finalize = books_collection_finalize;

    g_object_class_install_property (object_class,
                                     PROP_FILTER_TERM,
                                     g_param_spec_string ("filter-term",
                                                          "Collection filter term",
                                                          "Collection filter term",
                                                          NULL,
                                                          G_PARAM_READWRITE));

    g_type_class_add_private (klass, sizeof(BooksCollectionPrivate));
}

static void
books_collection_init (BooksCollection *collection)
{
    BooksCollectionPrivate *priv;
    gchar *db_path;
    gchar *db_error;

    collection->priv = priv = BOOKS_COLLECTION_GET_PRIVATE (collection);
    priv->filter_term = NULL;

    /* Create model */
    priv->store = gtk_list_store_new (BOOKS_COLLECTION_N_COLUMNS,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING);

    priv->filtered = gtk_tree_model_filter_new (GTK_TREE_MODEL (priv->store), NULL);

    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (priv->filtered),
                                            (GtkTreeModelFilterVisibleFunc) row_visible,
                                            priv, NULL);

    priv->sorted = gtk_tree_model_sort_new_with_model (priv->filtered);

    /* Create database */
    db_path = g_build_path (G_DIR_SEPARATOR_S, g_get_user_cache_dir(), "books", "meta.db", NULL);
    sqlite3_open (db_path, &priv->db);
    g_free (db_path);

    if (sqlite3_exec (priv->db, "CREATE TABLE IF NOT EXISTS books (author TEXT, title TEXT, path TEXT)",
                      NULL, NULL, &db_error)) {
        g_warning (_("Could not create table: %s\n"), db_error);
        sqlite3_free (db_error);
    }

    if (sqlite3_exec (priv->db, "SELECT author, title, path FROM books",
                      insert_row_into_model, priv->store, &db_error)) {
        g_warning (_("Could not select data: %s\n"), db_error);
        sqlite3_free (db_error);
    }
}
