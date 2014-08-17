#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <webkit/webkit.h>

#include "books-window.h"
#include "books-preferences-dialog.h"
#include "books-epub.h"


G_DEFINE_TYPE(BooksWindow, books_window, GTK_TYPE_WINDOW)

#define BOOKS_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), BOOKS_TYPE_WINDOW, BooksWindowPrivate))


struct _BooksWindowPrivate {
    GSettings *settings;
    GtkWidget *main_box;
    GtkWidget *toolbar;
    GtkWidget *scrolled_window;
    GtkWidget *html_view;
    GtkWidget *go_forward_item;
    GtkWidget *go_back_item;
    BooksEpub *epub;
    gchar     *css_uri;
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
                      "user-stylesheet-uri", priv->css_uri,
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
        WebKitDOMDocument *document;
        WebKitDOMStyleSheetList *sheet_list;
        const gchar *load_uri;
        const gchar *current_uri;
        guint i;

        current_uri = books_epub_get_uri (priv->epub);
        load_uri = webkit_web_view_get_uri (view);

        if (g_strcmp0 (current_uri, load_uri))
            books_epub_set_uri (priv->epub, load_uri);

        document = webkit_web_view_get_dom_document (view);
        sheet_list = webkit_dom_document_get_style_sheets (document);

        for (i = 0; i < webkit_dom_style_sheet_list_get_length (sheet_list); i++) {
            WebKitDOMStyleSheet *style_sheet;

            style_sheet = webkit_dom_style_sheet_list_item (sheet_list, i);
            webkit_dom_style_sheet_set_disabled (style_sheet, TRUE);
        }
    }
}

static void
on_window_destroy (GtkWidget *widget,
                   BooksWindowPrivate *priv)
{
    GtkAllocation allocation;

    gtk_widget_get_allocation (widget, &allocation);
    g_settings_set (priv->settings, "viewer-window-size",
                    "(ii)", allocation.width, allocation.height);
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
    BooksWindowPrivate *priv;

    priv = BOOKS_WINDOW_GET_PRIVATE (object);

    if (priv->css_uri != NULL) {
        g_free (priv->css_uri);
        priv->css_uri = NULL;
    }

    G_OBJECT_CLASS (books_window_parent_class)->finalize (object);
}

static void
books_window_class_init (BooksWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = books_window_dispose;
    object_class->finalize = books_window_finalize;

    g_type_class_add_private (klass, sizeof(BooksWindowPrivate));
}

static void
books_window_init (BooksWindow *window)
{
    BooksWindowPrivate *priv;
    guint width, height;

    window->priv = priv = BOOKS_WINDOW_GET_PRIVATE (window);

    g_signal_connect (window, "destroy",
                      G_CALLBACK (on_window_destroy), priv);

    /* Load window geometry */
    priv->settings = g_settings_new ("com.github.matze.books");
    g_settings_get (priv->settings, "viewer-window-size", "(ii)", &width, &height);
    gtk_window_set_default_size (GTK_WINDOW (window), width, height);

    priv->epub = NULL;
    priv->main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (window), priv->main_box);

    /* Add toolbar */
    priv->toolbar = gtk_toolbar_new ();
    gtk_container_add (GTK_CONTAINER (priv->main_box), priv->toolbar);

    priv->go_back_item = GTK_WIDGET (gtk_tool_button_new (NULL, NULL));
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (priv->go_back_item), "go-previous");
    gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), GTK_TOOL_ITEM (priv->go_back_item), -1);
    gtk_widget_set_sensitive (priv->go_back_item, FALSE);

    priv->go_forward_item = GTK_WIDGET (gtk_tool_button_new (NULL, NULL));
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (priv->go_forward_item), "go-next");
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

    /* Create CSS uri if requested */
    if (g_settings_get_enum (priv->settings, "style-sheet") == BOOKS_STYLE_SHEET_BOOKS) {
        gchar *css_filename;

        css_filename = g_build_filename (DATADIR, "books", "books.css", NULL);
        priv->css_uri = g_filename_to_uri (css_filename, NULL, NULL);

        g_free (css_filename);
    }
    else
        priv->css_uri = NULL;
}

