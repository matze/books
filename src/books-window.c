#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <webkit/webkit.h>

#include "books-window.h"
#include "books-epub.h"


G_DEFINE_TYPE(BooksWindow, books_window, GTK_TYPE_WINDOW)

#define BOOKS_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), BOOKS_TYPE_WINDOW, BooksWindowPrivate))


struct _BooksWindowPrivate {
    GtkWidget *main_box;
    GtkWidget *toolbar;
    GtkWidget *scrolled_window;
    GtkWidget *html_view;
    GtkWidget *go_forward_item;
    GtkWidget *go_back_item;

    GtkBuilder  *builder;

    BooksEpub   *epub;
};

static void load_web_view_content       (BooksWindowPrivate *priv);
static void update_navigation_buttons   (BooksWindowPrivate *priv);


GtkWidget *
books_window_new (void)
{
    return GTK_WIDGET (g_object_new (BOOKS_TYPE_WINDOW, NULL));
}

static void
load_web_view_content (BooksWindowPrivate *priv)
{
    gchar *uri;

    uri = books_epub_get_uri (priv->epub);

    if (uri != NULL) {
        WebKitWebSettings *settings;

        webkit_web_view_load_uri (WEBKIT_WEB_VIEW (priv->html_view), uri);

        settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (priv->html_view));
        g_object_set (G_OBJECT (settings),
                      "default-font-family", "serif",
                      NULL);
    }

    g_free (uri);
    update_navigation_buttons (priv);
}

static void
update_navigation_buttons (BooksWindowPrivate *priv)
{
    gtk_widget_set_sensitive (priv->go_back_item,
                              !books_epub_is_first (priv->epub));

    gtk_widget_set_sensitive (priv->go_forward_item,
                              !books_epub_is_last (priv->epub));
}

static void
on_add_ebook_button_clicked (GtkToolButton *button,
                             BooksWindow *parent)
{
    GtkWidget *file_chooser;

    file_chooser = gtk_file_chooser_dialog_new ("Open EPUB", GTK_WINDOW (parent),
                                                GTK_FILE_CHOOSER_ACTION_OPEN,
                                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                                NULL);

    if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) {
        BooksWindowPrivate *priv;
        gchar *filename;
        GError *error = NULL;

        priv = parent->priv;
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));

        if (priv->epub != NULL)
            g_object_unref (priv->epub);

        priv->epub = books_epub_new ();

        if (!books_epub_open (priv->epub, filename, &error)) {
            g_printerr ("EPUB open failed: %s\n", error->message);
            g_object_unref (priv->epub);
            priv->epub = NULL;
        }
        else {
            load_web_view_content (priv);
        }
    }

    gtk_widget_destroy (file_chooser);
}

static void
on_go_back_clicked (GtkToolButton *button,
                    BooksWindow *parent)
{
    BooksWindowPrivate *priv;
    gchar *content;

    priv = parent->priv;
    books_epub_previous (priv->epub);
    load_web_view_content (priv);
}

static void
on_go_forward_clicked (GtkToolButton *button,
                       BooksWindow *parent)
{
    BooksWindowPrivate *priv;
    gchar *content;

    priv = parent->priv;
    books_epub_next (priv->epub);
    load_web_view_content (priv);
}

static void
books_window_dispose (GObject *object)
{
    BooksWindowPrivate *priv;

    priv = BOOKS_WINDOW_GET_PRIVATE (object);

    if (priv->epub != NULL) {
        g_object_unref (priv->epub);
        priv->epub = NULL;
    }

    G_OBJECT_CLASS (books_window_parent_class)->dispose (object);
}

static void
books_window_finalize (GObject *object)
{
    G_OBJECT_CLASS (books_window_parent_class)->finalize (object);
}

static void
books_window_set_property (GObject *object,
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
books_window_get_property(GObject *object,
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
books_window_class_init (BooksWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->set_property = books_window_set_property;
    object_class->get_property = books_window_get_property;
    object_class->dispose = books_window_dispose;
    object_class->finalize = books_window_finalize;

    g_type_class_add_private (klass, sizeof(BooksWindowPrivate));
}

static void
books_window_init (BooksWindow *window)
{
    BooksWindowPrivate *priv;
    GtkToolItem *add_ebook_item;

    window->priv = priv = BOOKS_WINDOW_GET_PRIVATE (window);

    priv->main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (window), priv->main_box);

    /* Add toolbar */
    priv->toolbar = gtk_toolbar_new ();
    gtk_container_add (GTK_CONTAINER (priv->main_box), priv->toolbar);

    add_ebook_item = gtk_tool_button_new_from_stock (GTK_STOCK_ADD);
    gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), add_ebook_item, -1);

    priv->go_back_item = GTK_WIDGET (gtk_tool_button_new_from_stock (GTK_STOCK_GO_BACK));
    gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), GTK_TOOL_ITEM (priv->go_back_item), -1);
    gtk_widget_set_sensitive (priv->go_back_item, FALSE);

    priv->go_forward_item = GTK_WIDGET (gtk_tool_button_new_from_stock (GTK_STOCK_GO_FORWARD));
    gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), GTK_TOOL_ITEM (priv->go_forward_item), -1);
    gtk_widget_set_sensitive (priv->go_forward_item, FALSE);

    g_signal_connect (add_ebook_item, "clicked",
                      G_CALLBACK (on_add_ebook_button_clicked), window);

    g_signal_connect (priv->go_back_item, "clicked",
                      G_CALLBACK (on_go_back_clicked), window);

    g_signal_connect (priv->go_forward_item, "clicked",
                      G_CALLBACK (on_go_forward_clicked), window);

    /* Add EPUB view */
    priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (priv->main_box), priv->scrolled_window);

    priv->html_view = webkit_web_view_new ();
    gtk_widget_set_vexpand (priv->html_view, TRUE);
    gtk_container_add (GTK_CONTAINER (priv->scrolled_window), priv->html_view);

    /* Initialize builder */
    priv->builder = gtk_builder_new ();

    priv->epub = NULL;
}

