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

static void action_quit (GtkAction *, BooksMainWindow *window);
static void action_add_book (GtkAction *, BooksMainWindow *window);
static void action_remove_selected_book (GtkAction *, BooksMainWindow *window);
static void action_info (GtkAction *, BooksMainWindow *window);

struct _BooksMainWindowPrivate {
    GtkUIManager    *manager;
    GtkWidget       *main_box;
    GtkContainer    *list_scroll;
    GtkContainer    *icon_scroll;
    GtkActionGroup  *action_group;
    GtkEntry        *filter_entry;

    GtkWidget       *view;
    GtkTreeView     *tree_view;
    GtkIconView     *icon_view;

    gint             width;
    gint             height;

    BooksCollection *collection;
};

static GtkActionEntry action_entries[] = {
    { "Books", NULL, N_("Books") },
    { "Edit",  NULL, N_("Edit") },
    { "View",  NULL, N_("View") },
    { "Help",  NULL, N_("Help") },

    { "BookAdd", GTK_STOCK_ADD, N_("Add Books..."), "<control>O",
      N_("Add a book to the collection"),
      G_CALLBACK (action_add_book) },
    { "BookRemove", GTK_STOCK_REMOVE, N_("Remove Book"), "Delete",
      N_("Remove selected book from the collection"),
      G_CALLBACK (action_remove_selected_book) },

    { "BooksQuit", GTK_STOCK_ADD, N_("Quit"), "<control>Q",
      N_("Quit"),
      G_CALLBACK (action_quit) },

    { "BooksInfo", GTK_STOCK_ABOUT, N_("Info"), "",
      N_("Show information about Books"),
      G_CALLBACK (action_info) },
};

enum {
    VIEW_ICONS = 0,
    VIEW_LIST
};

static GtkRadioActionEntry view_entries[] = {
    /* Same terminology as in Nautilus */
    { "ViewIcon", GTK_STOCK_REMOVE, N_("Symbols"), "<control>1",
      N_("Show books in a grid with book covers"),
      VIEW_ICONS },
    { "ViewList", GTK_STOCK_REMOVE, N_("List"), "<control>2",
      N_("Show books in a list"),
      VIEW_LIST },
};

static gint n_action_entries = G_N_ELEMENTS (action_entries);
static gint n_view_entries = G_N_ELEMENTS (view_entries);

GtkWidget *
books_main_window_new (void)
{
    return GTK_WIDGET (g_object_new (BOOKS_TYPE_MAIN_WINDOW, NULL));
}

static void
action_view_changed (GtkRadioAction *action,
                     GtkRadioAction *current,
                     BooksMainWindow *window)
{
    BooksMainWindowPrivate *priv;

    priv = window->priv;

    if (!g_strcmp0 (gtk_action_get_name (GTK_ACTION (current)), "ViewIcon")) {
        gtk_widget_show (GTK_WIDGET (priv->icon_scroll));
        gtk_widget_hide (GTK_WIDGET (priv->list_scroll));
    }
    else {
        gtk_widget_show (GTK_WIDGET (priv->list_scroll));
        gtk_widget_hide (GTK_WIDGET (priv->icon_scroll));
    }
}

static void
import_book (gchar *path,
             BooksMainWindowPrivate *priv)
{
    BooksEpub *epub;
    GError *error = NULL;

    epub = books_epub_new ();

    if (books_epub_open (epub, path, &error))
        books_collection_add_book (priv->collection, epub, path);
    else
        g_printerr ("%s\n", error->message);

    g_object_unref (epub);
}

static void
action_add_book (GtkAction *action,
                 BooksMainWindow *window)
{
    GtkWidget *chooser;
    GtkFileFilter *filter;

    filter = gtk_file_filter_new ();
    gtk_file_filter_add_pattern (filter, "*.epub");

    chooser = gtk_file_chooser_dialog_new (_("Open EPUB"), GTK_WINDOW (window),
                                           GTK_FILE_CHOOSER_ACTION_OPEN,
                                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                           GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                           NULL);

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (chooser), TRUE);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (chooser), filter);

    if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT) {
        BooksMainWindowPrivate *priv;
        GSList *filenames;

        priv = window->priv;
        filenames = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (chooser));
        g_slist_foreach (filenames, (GFunc) import_book, priv);
        g_slist_free_full (filenames, g_free);
    }

    gtk_widget_destroy (chooser);
}

static void
remove_book_from_icon_view (GtkIconView *icon_view,
                            GtkTreePath *path,
                            BooksMainWindowPrivate *priv)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = books_collection_get_model (priv->collection);

    if (gtk_tree_model_get_iter (model, &iter, path))
        books_collection_remove_book (priv->collection, &iter);
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

    if (priv->view == GTK_WIDGET (priv->tree_view)) {
        selection = gtk_tree_view_get_selection (priv->tree_view);

        if (gtk_tree_selection_get_selected (selection, &model, &iter))
            books_collection_remove_book (priv->collection, &iter);
    }
    else {
        gtk_icon_view_selected_foreach (priv->icon_view, (GtkIconViewForeachFunc) remove_book_from_icon_view, priv);
    }
}

static void
action_quit (GtkAction *action,
             BooksMainWindow *window)
{
    gtk_main_quit ();
}

static void
action_info (GtkAction *action,
             BooksMainWindow *window)
{
    static const gchar *authors[] = {
        "Matthias Vogelgesang http://bloerg.net",
        NULL
    };

    static const gchar *artists[] = {
        "Charles Riccardi, from the Noun Project http://thenounproject.com/noun/book/#icon-No2051",
        NULL
    };

    gtk_show_about_dialog (NULL,
                           "title", _("About Books"),
                           "comments", _("Books is an e-book manager and viewer."),
                           "authors", authors,
                           "artists", artists,
                           "website", "http://matze.github.com/books",
                           "license-type", GTK_LICENSE_GPL_3_0,
                           "version", PACKAGE_VERSION,
                           NULL);
}

static void
open_selected_book (BooksMainWindowPrivate *priv,
                    GtkTreePath *path)
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

static void
on_row_activated (GtkTreeView *view,
                  GtkTreePath *path,
                  GtkTreeViewColumn *col,
                  BooksMainWindowPrivate *priv)
{
    open_selected_book (priv, path);
}

static void
on_item_activated (GtkIconView *icon_view,
                   GtkTreePath *path,
                   BooksMainWindowPrivate *priv)
{
    open_selected_book (priv, path);
}

static void
on_window_resize (GtkContainer *container,
                  BooksMainWindowPrivate *priv)
{
    gint width;
    gint height;

    gtk_window_get_size (GTK_WINDOW (container), &width, &height);

    if (width != priv->width || height != priv->height) {
        priv->width = width;
        priv->height = height;

        if (priv->view == GTK_WIDGET (priv->icon_view)) {
            gtk_icon_view_set_columns (priv->icon_view, 0);
            gtk_icon_view_set_columns (priv->icon_view, -1);
        }
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
    GtkAccelGroup       *accel_group;
    GtkWidget           *toolbar;
    GtkWidget           *menubar;
    GtkToolItem         *separator_item;
    GtkToolItem         *filter_item;
    GtkTreeModel        *model;
    GtkTreeViewColumn   *author_column;
    GtkTreeViewColumn   *title_column;
    GtkCellRenderer     *renderer;
    GtkTreeSelection    *selection;
    GtkContainer        *scroll_box;
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
    gtk_action_group_add_radio_actions (priv->action_group,
                                        view_entries, n_view_entries, VIEW_ICONS,
                                        G_CALLBACK (action_view_changed), window);

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
        g_warning ("%s\n", error->message);
        g_error_free (error);
    }

    g_bytes_unref (bytes);

    accel_group = gtk_ui_manager_get_accel_group (priv->manager);
    gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

    /* Create widgets */
    priv->main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    /* Add menu bar */
    menubar = gtk_ui_manager_get_widget (priv->manager, "/MenuBar");

    /* Add toolbar */
    toolbar = gtk_ui_manager_get_widget (priv->manager, "/ToolBar");
    gtk_style_context_add_class (gtk_widget_get_style_context (toolbar),
                                 GTK_STYLE_CLASS_PRIMARY_TOOLBAR);

    separator_item = gtk_separator_tool_item_new ();
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), separator_item, -1);
    gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (separator_item), FALSE);
    gtk_tool_item_set_expand (GTK_TOOL_ITEM (separator_item), TRUE);

    filter_item = gtk_tool_item_new ();
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), filter_item, -1);
    priv->filter_entry = GTK_ENTRY (gtk_entry_new ());

    g_object_bind_property (priv->filter_entry, "text",
                            priv->collection, "filter-term",
                            0);

    /* Create book view */
    scroll_box = GTK_CONTAINER (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));

    model = books_collection_get_model (priv->collection);
    priv->tree_view = GTK_TREE_VIEW (gtk_tree_view_new_with_model (model));
    g_object_ref (priv->tree_view);
    gtk_widget_set_vexpand (GTK_WIDGET (priv->tree_view), TRUE);

    renderer = gtk_cell_renderer_text_new ();

    author_column = gtk_tree_view_column_new_with_attributes (_("Author"), renderer,
            "text", BOOKS_COLLECTION_AUTHOR_COLUMN,
            NULL);

    gtk_tree_view_column_set_sort_column_id (author_column, BOOKS_COLLECTION_AUTHOR_COLUMN);
    gtk_tree_view_append_column (priv->tree_view, author_column);

    title_column = gtk_tree_view_column_new_with_attributes (_("Title"), renderer,
            "text", BOOKS_COLLECTION_TITLE_COLUMN,
            NULL);

    gtk_tree_view_column_set_sort_column_id (title_column, BOOKS_COLLECTION_TITLE_COLUMN);
    gtk_tree_view_append_column (priv->tree_view, title_column);

    selection = gtk_tree_view_get_selection (priv->tree_view);
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

    /* Create icon view */
    priv->icon_view = GTK_ICON_VIEW (gtk_icon_view_new_with_model (model));
    g_object_ref (priv->icon_view);

    gtk_widget_set_vexpand (GTK_WIDGET (priv->icon_view), TRUE);
    priv->view = GTK_WIDGET (priv->icon_view);

    gtk_icon_view_set_markup_column (priv->icon_view, BOOKS_COLLECTION_MARKUP_COLUMN);
    gtk_icon_view_set_pixbuf_column (priv->icon_view, BOOKS_COLLECTION_ICON_COLUMN);

    priv->list_scroll = GTK_CONTAINER (gtk_scrolled_window_new (NULL, NULL));
    priv->icon_scroll = GTK_CONTAINER (gtk_scrolled_window_new (NULL, NULL));

    /* Layout widgets */
    gtk_container_add (GTK_CONTAINER (window), priv->main_box);
    gtk_container_add (GTK_CONTAINER (priv->main_box), menubar);
    gtk_container_add (GTK_CONTAINER (priv->main_box), toolbar);
    gtk_container_add (GTK_CONTAINER (priv->main_box), GTK_WIDGET (scroll_box));

    gtk_container_add (GTK_CONTAINER (filter_item), GTK_WIDGET (priv->filter_entry));

    gtk_container_add (GTK_CONTAINER (scroll_box), GTK_WIDGET (priv->list_scroll));
    gtk_container_add (GTK_CONTAINER (scroll_box), GTK_WIDGET (priv->icon_scroll));

    gtk_container_add (GTK_CONTAINER (priv->icon_scroll), GTK_WIDGET (priv->icon_view));
    gtk_container_add (GTK_CONTAINER (priv->list_scroll), GTK_WIDGET (priv->tree_view));

    /* Show widgets */
    gtk_widget_show (priv->main_box);
    gtk_widget_show_all (toolbar);
    gtk_widget_show_all (menubar);
    gtk_widget_show (GTK_WIDGET (scroll_box));
    gtk_widget_show (GTK_WIDGET (priv->icon_scroll));
    gtk_widget_show (GTK_WIDGET (priv->icon_view));
    gtk_widget_show (GTK_WIDGET (priv->tree_view));

    /* Connect signals */
    g_signal_connect (priv->tree_view, "row-activated",
                      G_CALLBACK (on_row_activated), priv);

    g_signal_connect (priv->icon_view, "item-activated",
                      G_CALLBACK (on_item_activated), priv);

    g_signal_connect (window, "check-resize",
                      G_CALLBACK (on_window_resize), priv);
}
