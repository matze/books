#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "books-main-window.h"
#include "books-window.h"
#include "books-collection.h"


G_DEFINE_TYPE(BooksMainWindow, books_main_window, GTK_TYPE_WINDOW)

#define BOOKS_MAIN_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), BOOKS_TYPE_MAIN_WINDOW, BooksMainWindowPrivate))


struct _BooksMainWindowPrivate {
    GtkWidget       *main_box;
    GtkWidget       *toolbar;
    GtkEntry        *filter_entry;

    GtkTreeView     *books_view;
    BooksCollection *collection;
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
    GError *error = NULL;

    file_chooser = gtk_file_chooser_dialog_new (_("Open EPUB"), GTK_WINDOW (parent),
                                                GTK_FILE_CHOOSER_ACTION_OPEN,
                                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                                NULL);

    if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) {
        BooksMainWindowPrivate *priv;
        BooksEpub *epub;
        gchar *filename;

        priv = parent->priv;
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
        epub = books_epub_new ();

        if (books_epub_open (epub, filename, &error))
            books_collection_add_book (priv->collection, epub, filename);
        else
            g_printerr ("%s\n", error->message);

        g_free (filename);
        g_object_unref (epub);
    }

    gtk_widget_destroy (file_chooser);
}

static void
remove_currently_selected_book (BooksMainWindowPrivate *priv)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;

    selection = gtk_tree_view_get_selection (priv->books_view);

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
        books_collection_remove_book (priv->collection, &iter);
}

static void
on_remove_ebook_button_clicked (GtkToolButton *button,
                                BooksMainWindowPrivate *priv)
{
    remove_currently_selected_book (priv);
}

static void
on_row_activated (GtkTreeView *view,
                  GtkTreePath *path,
                  GtkTreeViewColumn *col,
                  BooksMainWindowPrivate *priv)
{
    GtkTreeIter iter;
    GtkTreePath *filtered_path;
    GtkTreePath *child_path;
    BooksEpub *epub;
    GError *error = NULL;

    epub = books_collection_get_book (priv->collection, path, &error);

    if (epub != NULL) {
        GtkWidget *book_window;

        book_window = books_window_new ();
        books_window_set_epub (BOOKS_WINDOW (book_window), epub);
        gtk_widget_set_size_request (book_window, 594, 841);
        gtk_widget_show_all (book_window);
    }
}

static gboolean
on_books_view_key_press (GtkWidget *widget,
                         GdkEventKey *event,
                         BooksMainWindowPrivate *priv)
{
    if (event->keyval == GDK_KEY_Delete) {
        remove_currently_selected_book (priv);
        return TRUE;
    }

    return FALSE;
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
books_main_window_class_init (BooksMainWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = books_main_window_dispose;
    object_class->finalize = books_main_window_finalize;

    g_type_class_add_private (klass, sizeof(BooksMainWindowPrivate));
}

static void
books_main_window_init (BooksMainWindow *window)
{
    BooksMainWindowPrivate *priv;
    GtkToolItem         *add_ebook_item;
    GtkToolItem         *remove_ebook_item;
    GtkToolItem         *separator_item;
    GtkToolItem         *filter_item;
    GtkContainer        *scrolled;
    GtkTreeModel        *model;
    GtkTreeViewColumn   *author_column;
    GtkTreeViewColumn   *title_column;
    GtkCellRenderer     *renderer;
    GtkTreeSelection    *selection;

    window->priv = priv = BOOKS_MAIN_WINDOW_GET_PRIVATE (window);

    /* Create book collection */
    priv->collection = books_collection_new ();

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

    remove_ebook_item = gtk_tool_button_new_from_stock (GTK_STOCK_REMOVE);
    gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), remove_ebook_item, -1);
    gtk_tool_item_set_tooltip_text (remove_ebook_item, _("Remove EPUB"));

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

    g_signal_connect (remove_ebook_item, "clicked",
                      G_CALLBACK (on_remove_ebook_button_clicked), priv);

    g_object_bind_property (priv->filter_entry, "text",
                            priv->collection, "filter-term",
                            0);

    /* ... and its view */
    scrolled = GTK_CONTAINER (gtk_scrolled_window_new (NULL, NULL));
    gtk_container_add (GTK_CONTAINER (priv->main_box), GTK_WIDGET (scrolled));

    model = books_collection_get_model (priv->collection);
    priv->books_view = GTK_TREE_VIEW (gtk_tree_view_new_with_model (model));
    gtk_container_add (GTK_CONTAINER (scrolled), GTK_WIDGET (priv->books_view));
    gtk_widget_set_vexpand (GTK_WIDGET (priv->books_view), TRUE);

    renderer = gtk_cell_renderer_text_new ();

    author_column = gtk_tree_view_column_new_with_attributes (_("Author"), renderer,
            "text", BOOKS_COLLECTION_AUTHOR_COLUMN,
            NULL);

    gtk_tree_view_column_set_sort_column_id (author_column, BOOKS_COLLECTION_AUTHOR_COLUMN);
    gtk_tree_view_append_column (priv->books_view, author_column);

    title_column = gtk_tree_view_column_new_with_attributes (_("Title"), renderer,
            "text", BOOKS_COLLECTION_TITLE_COLUMN,
            NULL);

    gtk_tree_view_column_set_sort_column_id (title_column, BOOKS_COLLECTION_TITLE_COLUMN);
    gtk_tree_view_append_column (priv->books_view, title_column);

    selection = gtk_tree_view_get_selection (priv->books_view);
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

    g_signal_connect (priv->books_view, "row-activated",
                      G_CALLBACK (on_row_activated), priv);

    g_signal_connect (priv->books_view, "key-press-event",
                      G_CALLBACK (on_books_view_key_press), priv);
}

