#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <webkit/webkit.h>

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

    GtkTreeView     *books_view;
    GtkListStore    *books;
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

    file_chooser = gtk_file_chooser_dialog_new ("Open EPUB", GTK_WINDOW (parent),
                                                GTK_FILE_CHOOSER_ACTION_OPEN,
                                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                                NULL);

    if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) {
        BooksMainWindowPrivate *priv;
        BooksEpub *epub;
        GtkTreeIter iter;
        gchar *filename;
        GError *error = NULL;

        priv = parent->priv;
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));

        epub = books_epub_new ();
        books_epub_open (epub, filename, &error);

        gtk_list_store_append (priv->books, &iter);
        gtk_list_store_set (priv->books, &iter,
                            COLUMN_AUTHOR, books_epub_get_meta (epub, "creator"),
                            COLUMN_TITLE, books_epub_get_meta (epub, "title"),
                            COLUMN_PATH, filename,
                            -1);

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
    BooksMainWindowPrivate *priv;

    priv = window->priv;

    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->books), &iter, path)) {
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

static void
books_main_window_init (BooksMainWindow *window)
{
    BooksMainWindowPrivate *priv;
    GtkToolItem *add_ebook_item;
    GtkCellRenderer *renderer;
    GtkTreeSelection *selection;

    window->priv = priv = BOOKS_MAIN_WINDOW_GET_PRIVATE (window);

    priv->main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (window), priv->main_box);

    /* Add toolbar */
    priv->toolbar = gtk_toolbar_new ();
    gtk_container_add (GTK_CONTAINER (priv->main_box), priv->toolbar);

    add_ebook_item = gtk_tool_button_new_from_stock (GTK_STOCK_ADD);
    gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), add_ebook_item, -1);

    g_signal_connect (add_ebook_item, "clicked",
                      G_CALLBACK (on_add_ebook_button_clicked), window);

    /* Create model */
    priv->books = gtk_list_store_new (N_COLUMNS,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING);

    /* ... and its view */
    priv->books_view = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (priv->books)));
    gtk_container_add (GTK_CONTAINER (priv->main_box), GTK_WIDGET (priv->books_view));
    gtk_widget_set_vexpand (GTK_WIDGET (priv->books_view), TRUE);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_insert_column_with_attributes (priv->books_view, -1, "Author", renderer,
                                                 "text", COLUMN_AUTHOR,
                                                 NULL);

    gtk_tree_view_insert_column_with_attributes (priv->books_view, -1, "Title", renderer,
                                                 "text", COLUMN_TITLE,
                                                 NULL);

    selection = gtk_tree_view_get_selection (priv->books_view);
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

    g_signal_connect (priv->books_view, "row-activated",
                      G_CALLBACK (on_row_activated), window);
}

