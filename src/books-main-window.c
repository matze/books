#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <sqlite3.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "books-main-window.h"
#include "books-window.h"


G_DEFINE_TYPE(BooksMainWindow, books_main_window, GTK_TYPE_WINDOW)

#define BOOKS_MAIN_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), BOOKS_TYPE_MAIN_WINDOW, BooksMainWindowPrivate))


enum {
    COLUMN_AUTHOR,
    COLUMN_TITLE,
    COLUMN_PATH,
    N_COLUMNS
};

struct _BooksMainWindowPrivate {
    GtkWidget       *main_box;
    GtkWidget       *toolbar;
    GtkEntry        *filter_entry;

    GtkTreeView     *books_view;
    GtkListStore    *books;
    GtkTreeModel    *filtered_books;
    GtkTreeModel    *sorted_books;

    sqlite3         *db;
};

GtkWidget *
books_main_window_new (void)
{
    return GTK_WIDGET (g_object_new (BOOKS_TYPE_MAIN_WINDOW, NULL));
}

static void
on_add_ebook_button_clicked (GtkToolButton *button,
                             BooksMainWindow *parent)
{
    GtkWidget *file_chooser;

    file_chooser = gtk_file_chooser_dialog_new (_("Open EPUB"), GTK_WINDOW (parent),
                                                GTK_FILE_CHOOSER_ACTION_OPEN,
                                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                                NULL);

    if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) {
        BooksMainWindowPrivate *priv;
        BooksEpub *epub;
        GtkTreeIter iter;
        gchar *filename;
        gchar *sql_error;
        const gchar *author;
        const gchar *title;
        const gchar *insert_sql = "INSERT INTO books (author, title, path) VALUES (?, ?, ?)";
        sqlite3_stmt *insert_stmt = NULL;
        GError *error = NULL;

        priv = parent->priv;
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));

        epub = books_epub_new ();
        books_epub_open (epub, filename, &error);
        author = books_epub_get_meta (epub, "creator");
        title = books_epub_get_meta (epub, "title");

        gtk_list_store_append (priv->books, &iter);
        gtk_list_store_set (priv->books, &iter,
                            COLUMN_AUTHOR, author,
                            COLUMN_TITLE, title,
                            COLUMN_PATH, filename,
                            -1);

        sqlite3_prepare_v2 (priv->db, insert_sql, -1, &insert_stmt, NULL);
        sqlite3_bind_text (insert_stmt, 1, author, strlen (author), NULL);
        sqlite3_bind_text (insert_stmt, 2, title, strlen (title), NULL);
        sqlite3_bind_text (insert_stmt, 3, filename, strlen (filename), NULL);
        sqlite3_step (insert_stmt);
        sqlite3_finalize (insert_stmt);

        g_object_unref (epub);
    }

    gtk_widget_destroy (file_chooser);
}

static void
on_row_activated (GtkTreeView *view,
                  GtkTreePath *path,
                  GtkTreeViewColumn *col,
                  BooksMainWindow *window)
{
    GtkTreeIter iter;
    GtkTreePath *filtered_path;
    GtkTreePath *child_path;
    BooksMainWindowPrivate *priv;

    priv = window->priv;

    filtered_path = gtk_tree_model_sort_convert_path_to_child_path (GTK_TREE_MODEL_SORT (priv->sorted_books),
                                                                    path);

    child_path = gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (priv->filtered_books),
                                                                   filtered_path);

    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->books), &iter, child_path)) {
        BooksEpub *epub;
        BooksWindow *view;
        gchar *path;
        GError *error = NULL;

        gtk_tree_model_get (GTK_TREE_MODEL (priv->books), &iter,
                            COLUMN_PATH, &path,
                            -1);

        epub = books_epub_new ();
        books_epub_open (epub, path, &error);

        if (error != NULL) {
            g_object_unref (epub);
            g_printerr ("%s\n", error->message);
            return;
        }

        view = BOOKS_WINDOW (books_window_new ());
        books_window_set_epub (view, epub);
        gtk_widget_set_size_request (GTK_WIDGET (view), 594, 841);
        gtk_widget_show_all (GTK_WIDGET (view));
    }
}

static void
books_main_window_dispose (GObject *object)
{
    G_OBJECT_CLASS (books_main_window_parent_class)->dispose (object);
}

static void
books_main_window_finalize (GObject *object)
{
    BooksMainWindowPrivate *priv;

    priv = BOOKS_MAIN_WINDOW_GET_PRIVATE (object);
    sqlite3_close (priv->db);

    G_OBJECT_CLASS (books_main_window_parent_class)->finalize (object);
}

static void
books_main_window_set_property (GObject *object,
                         guint property_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
books_main_window_get_property(GObject *object,
                        guint property_id,
                        GValue *value,
                        GParamSpec *pspec)
{
    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}


static void
books_main_window_class_init (BooksMainWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->set_property = books_main_window_set_property;
    object_class->get_property = books_main_window_get_property;
    object_class->dispose = books_main_window_dispose;
    object_class->finalize = books_main_window_finalize;

    g_type_class_add_private (klass, sizeof(BooksMainWindowPrivate));
}

static int
insert_row_into_model (gpointer user_data,
                       gint argc,
                       gchar **argv,
                       gchar **column)
{
    BooksMainWindowPrivate *priv;
    GtkTreeIter iter;

    g_assert (argc == 3);
    priv = (BooksMainWindowPrivate *) user_data;

    gtk_list_store_append (priv->books, &iter);
    gtk_list_store_set (priv->books, &iter,
                        COLUMN_AUTHOR, argv[0],
                        COLUMN_TITLE, argv[1],
                        COLUMN_PATH, argv[2],
                        -1);
    return 0;
}

static void
on_entry_insert (GtkEditable *editable,
                 BooksMainWindowPrivate *priv)
{
    gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filtered_books));
}

static gboolean
row_visible (GtkTreeModel *model,
             GtkTreeIter *iter,
             BooksMainWindowPrivate *priv)
{
    gboolean visible;
    gchar *lowered_author;
    gchar *lowered_title;
    gchar *lowered_entry;
    gchar *author;
    gchar *title;
    const gchar *entry;

    entry = gtk_entry_get_text (priv->filter_entry);

    if (entry == NULL)
        return TRUE;

    gtk_tree_model_get (model, iter,
                        COLUMN_AUTHOR, &author,
                        COLUMN_TITLE, &title,
                        -1);

    if (author == NULL || title == NULL)
        return TRUE;

    lowered_entry = g_utf8_strdown (entry, -1);
    lowered_author = g_utf8_strdown (author, -1);
    lowered_title = g_utf8_strdown (title, -1);

    visible = strstr (lowered_author, lowered_entry) != NULL ||
              strstr (lowered_title, lowered_entry) != NULL;

    g_free (lowered_author);
    g_free (lowered_title);
    g_free (lowered_entry);
    g_free (author);
    g_free (title);

    return visible;
}

static void
books_main_window_init (BooksMainWindow *window)
{
    BooksMainWindowPrivate *priv;
    GtkToolItem         *add_ebook_item;
    GtkToolItem         *separator_item;
    GtkToolItem         *filter_item;
    GtkContainer        *scrolled;
    GtkTreeViewColumn   *author_column;
    GtkTreeViewColumn   *title_column;
    GtkCellRenderer     *renderer;
    GtkTreeSelection    *selection;
    gchar *db_path;
    gchar *db_error;

    window->priv = priv = BOOKS_MAIN_WINDOW_GET_PRIVATE (window);

    priv->main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (window), priv->main_box);

    /* Add toolbar */
    priv->toolbar = gtk_toolbar_new ();
    gtk_container_add (GTK_CONTAINER (priv->main_box), priv->toolbar);
    gtk_style_context_add_class (gtk_widget_get_style_context (priv->toolbar),
                                 GTK_STYLE_CLASS_PRIMARY_TOOLBAR);

    add_ebook_item = gtk_tool_button_new_from_stock (GTK_STOCK_ADD);
    gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), add_ebook_item, -1);
    gtk_tool_item_set_tooltip_text (add_ebook_item, _("Add EPUB"));

    separator_item = gtk_separator_tool_item_new ();
    gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), separator_item, -1);
    gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (separator_item), FALSE);
    gtk_tool_item_set_expand (GTK_TOOL_ITEM (separator_item), TRUE);

    filter_item = gtk_tool_item_new ();
    gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), filter_item, -1);
    priv->filter_entry = GTK_ENTRY (gtk_entry_new ());
    gtk_container_add (GTK_CONTAINER (filter_item), GTK_WIDGET (priv->filter_entry));

    g_signal_connect (add_ebook_item, "clicked",
                      G_CALLBACK (on_add_ebook_button_clicked), window);

    g_signal_connect (priv->filter_entry, "changed",
                      G_CALLBACK (on_entry_insert), priv);

    /* Create model */
    priv->books = gtk_list_store_new (N_COLUMNS,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING);

    priv->filtered_books = gtk_tree_model_filter_new (GTK_TREE_MODEL (priv->books), NULL);

    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (priv->filtered_books),
                                            (GtkTreeModelFilterVisibleFunc) row_visible,
                                            priv, NULL);

    priv->sorted_books = gtk_tree_model_sort_new_with_model (priv->filtered_books);

    /* ... and its view */
    scrolled = GTK_CONTAINER (gtk_scrolled_window_new (NULL, NULL));
    gtk_container_add (GTK_CONTAINER (priv->main_box), GTK_WIDGET (scrolled));

    priv->books_view = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (priv->sorted_books)));
    gtk_container_add (GTK_CONTAINER (scrolled), GTK_WIDGET (priv->books_view));
    gtk_widget_set_vexpand (GTK_WIDGET (priv->books_view), TRUE);

    renderer = gtk_cell_renderer_text_new ();

    author_column = gtk_tree_view_column_new_with_attributes (_("Author"), renderer,
            "text", COLUMN_AUTHOR,
            NULL);

    gtk_tree_view_column_set_sort_column_id (author_column, COLUMN_AUTHOR);
    gtk_tree_view_append_column (priv->books_view, author_column);

    title_column = gtk_tree_view_column_new_with_attributes (_("Title"), renderer,
            "text", COLUMN_TITLE,
            NULL);

    gtk_tree_view_column_set_sort_column_id (title_column, COLUMN_TITLE);
    gtk_tree_view_append_column (priv->books_view, title_column);

    selection = gtk_tree_view_get_selection (priv->books_view);
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

    g_signal_connect (priv->books_view, "row-activated",
                      G_CALLBACK (on_row_activated), window);

    /* Create data base */
    db_path = g_build_path (G_DIR_SEPARATOR_S, g_get_user_cache_dir(), "books", "meta.db", NULL);
    sqlite3_open (db_path, &priv->db);
    g_free (db_path);

    if (sqlite3_exec (priv->db, "CREATE TABLE IF NOT EXISTS books (author TEXT, title TEXT, path TEXT)",
                      NULL, NULL, &db_error)) {
        g_warning (_("Could not create table: %s\n"), db_error);
        sqlite3_free (db_error);
    }

    if (sqlite3_exec (priv->db, "SELECT author, title, path FROM books",
                      insert_row_into_model, priv, &db_error)) {
        g_warning (_("Could not select data: %s\n"), db_error);
        sqlite3_free (db_error);
    }
}

