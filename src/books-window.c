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
    BooksEpub *epub;
};

static void load_web_view_content       (BooksWindowPrivate *priv);
static void update_navigation_buttons   (BooksWindowPrivate *priv);


GtkWidget *
books_window_new (void)
{
    return GTK_WIDGET (g_object_new (BOOKS_TYPE_WINDOW, NULL));
}

void
books_window_set_epub (BooksWindow *window,
                       BooksEpub *epub)
{
    g_return_if_fail (BOOKS_IS_WINDOW (window));

    window->priv->epub = epub;
    load_web_view_content (window->priv);
}

static void
load_web_view_content (BooksWindowPrivate *priv)
{
    const gchar *uri;

    uri = books_epub_get_uri (priv->epub);

    if (uri != NULL) {
        WebKitWebSettings *settings;

        webkit_web_view_load_uri (WEBKIT_WEB_VIEW (priv->html_view), uri);

        settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (priv->html_view));
        g_object_set (G_OBJECT (settings),
                      "default-font-family", "serif",
                      NULL);
    }

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
on_go_back_clicked (GtkToolButton *button,
                    BooksWindowPrivate *priv)
{
    books_epub_previous (priv->epub);
    load_web_view_content (priv);
}

static void
on_go_forward_clicked (GtkToolButton *button,
                       BooksWindowPrivate *priv)
{
    books_epub_next (priv->epub);
    load_web_view_content (priv);
}

static void
on_load_status_changed (WebKitWebView *view,
                        GParamSpec *pspec,
                        BooksWindowPrivate *priv)
{
    WebKitLoadStatus load_status;

    load_status = webkit_web_view_get_load_status (view);

    if (load_status == WEBKIT_LOAD_FINISHED) {
        const gchar *load_uri;
        const gchar *current_uri;

        current_uri = books_epub_get_uri (priv->epub);
        load_uri = webkit_web_view_get_uri (view);

        if (g_strcmp0 (current_uri, load_uri))
            books_epub_set_uri (priv->epub, load_uri);
    }
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

    window->priv = priv = BOOKS_WINDOW_GET_PRIVATE (window);

    priv->main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (window), priv->main_box);

    /* Add toolbar */
    priv->toolbar = gtk_toolbar_new ();
    gtk_container_add (GTK_CONTAINER (priv->main_box), priv->toolbar);

    priv->go_back_item = GTK_WIDGET (gtk_tool_button_new_from_stock (GTK_STOCK_GO_BACK));
    gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), GTK_TOOL_ITEM (priv->go_back_item), -1);
    gtk_widget_set_sensitive (priv->go_back_item, FALSE);

    priv->go_forward_item = GTK_WIDGET (gtk_tool_button_new_from_stock (GTK_STOCK_GO_FORWARD));
    gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), GTK_TOOL_ITEM (priv->go_forward_item), -1);
    gtk_widget_set_sensitive (priv->go_forward_item, FALSE);

    g_signal_connect (priv->go_back_item, "clicked",
                      G_CALLBACK (on_go_back_clicked), priv);

    g_signal_connect (priv->go_forward_item, "clicked",
                      G_CALLBACK (on_go_forward_clicked), priv);

    /* Add EPUB view */
    priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (priv->main_box), priv->scrolled_window);

    priv->html_view = webkit_web_view_new ();
    gtk_widget_set_vexpand (priv->html_view, TRUE);
    gtk_container_add (GTK_CONTAINER (priv->scrolled_window), priv->html_view);

    g_signal_connect (priv->html_view, "notify::load-status",
                      G_CALLBACK (on_load_status_changed),
                      priv);

    priv->epub = NULL;
}

