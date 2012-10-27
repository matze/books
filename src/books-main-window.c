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

static void action_add_book (GtkAction *, BooksMainWindow *window);
static void action_remove_selected_book (GtkAction *, BooksMainWindow *window);

struct _BooksMainWindowPrivate {
    GtkUIManager    *manager;
    GtkWidget       *main_box;
    GtkWidget       *toolbar;
    GtkActionGroup  *action_group;
    GtkEntry        *filter_entry;

    GtkTreeView     *books_view;
    BooksCollection *collection;
};

static GtkActionEntry action_entries[] = {
    { "BookAdd", GTK_STOCK_ADD, N_("Add Book..."), "<control>O",
      N_("Add a book to the collection"),
      G_CALLBACK (action_add_book) },
    { "BookRemove", GTK_STOCK_REMOVE, N_("Remove Book"), "Delete",
      N_("Remove selected book from the collection"),
      G_CALLBACK (action_remove_selected_book) },
};

static gint n_action_entries = G_N_ELEMENTS (action_entries);

GtkWidget *
books_main_window_new (void)
{
    return GTK_WIDGET (g_object_new (BOOKS_TYPE_MAIN_WINDOW, NULL));
}

static void
action_add_book (GtkAction *action,
                 BooksMainWindow *window)
{
    GtkWidget *file_chooser;
    GError *error = NULL;

    file_chooser = gtk_file_chooser_dialog_new (_("Open EPUB"), GTK_WINDOW (window),
                                                GTK_FILE_CHOOSER_ACTION_OPEN,
                                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                                NULL);

    if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) {
        BooksMainWindowPrivate *priv;
        BooksEpub *epub;
        gchar *filename;

        priv = window->priv;
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
action_remove_selected_book (GtkAction *action,
                             BooksMainWindow *window)
{
    BooksMainWindowPrivate *priv;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;

    priv = window->priv;
    selection = gtk_tree_view_get_selection (priv->books_view);

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
        books_collection_remove_book (priv->collection, &iter);
}

static void
on_row_activated (GtkTreeView *view,
                  GtkTreePath *path,
                  GtkTreeViewColumn *col,
                  BooksMainWindowPrivate *priv)
{
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
        GtkAction *action;

        action = gtk_action_group_get_action (priv->action_group, "BookRemove");
        gtk_action_activate (action);
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
    GtkToolItem         *separator_item;
    GtkToolItem         *filter_item;
    GtkContainer        *scrolled;
    GtkTreeModel        *model;
    GtkTreeViewColumn   *author_column;
    GtkTreeViewColumn   *title_column;
    GtkCellRenderer     *renderer;
    GtkTreeSelection    *selection;
    GBytes              *bytes;
    gsize                size;
    const gchar         *ui_data;
    GError              *error = NULL;

    window->priv = priv = BOOKS_MAIN_WINDOW_GET_PRIVATE (window);

    /* Create book collection */
    priv->collection = books_collection_new ();

    /* Create actions */
    priv->action_group = gtk_action_group_new ("MainActions");
    gtk_action_group_set_translation_domain (priv->action_group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions (priv->action_group, action_entries, n_action_entries, window);

    priv->manager = gtk_ui_manager_new ();
    bytes = g_resources_lookup_data ("/com/github/matze/books/ui/books.xml", 0, &error);

    if (error != NULL) {
        g_error ("%s\n", error->message);
        g_error_free (error);
    }

    ui_data = (const gchar *) g_bytes_get_data (bytes, &size);
    gtk_ui_manager_insert_action_group (priv->manager, priv->action_group, -1);
    gtk_ui_manager_add_ui_from_string (priv->manager, ui_data, size, &error);

    if (error != NULL) {
        g_error ("%s\n", error->message);
        g_error_free (error);
    }

    g_bytes_unref (bytes);

    /* Create widgets */
    priv->main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (window), priv->main_box);

    /* Add toolbar */
    priv->toolbar = gtk_ui_manager_get_widget (priv->manager, "/ToolBar");
    gtk_container_add (GTK_CONTAINER (priv->main_box), priv->toolbar);
    gtk_style_context_add_class (gtk_widget_get_style_context (priv->toolbar),
                                 GTK_STYLE_CLASS_PRIMARY_TOOLBAR);

    separator_item = gtk_separator_tool_item_new ();
    gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), separator_item, -1);
    gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (separator_item), FALSE);
    gtk_tool_item_set_expand (GTK_TOOL_ITEM (separator_item), TRUE);

    filter_item = gtk_tool_item_new ();
    gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), filter_item, -1);
    priv->filter_entry = GTK_ENTRY (gtk_entry_new ());
    gtk_container_add (GTK_CONTAINER (filter_item), GTK_WIDGET (priv->filter_entry));

    g_object_bind_property (priv->filter_entry, "text",
                            priv->collection, "filter-term",
                            0);

    /* Create book view */
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
