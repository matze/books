#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>

#include "books-preferences-dialog.h"


G_DEFINE_TYPE(BooksPreferencesDialog, books_preferences_dialog, GTK_TYPE_DIALOG)

#define BOOKS_PREFERENCES_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), BOOKS_TYPE_PREFERENCES_DIALOG, BooksPreferencesDialogPrivate))

struct _BooksPreferencesDialogPrivate {
    GSettings *settings;

    GtkWidget *notebook;
};

static GtkWidget *preferences_dialog = NULL;


void
books_show_preferences_dialog (BooksMainWindow *parent)
{

    if (preferences_dialog == NULL) {
        preferences_dialog = GTK_WIDGET (g_object_new (BOOKS_TYPE_PREFERENCES_DIALOG, NULL));
        g_signal_connect (preferences_dialog, "destroy", G_CALLBACK (gtk_widget_destroyed), &preferences_dialog);
    }

    if (GTK_WINDOW (parent) != gtk_window_get_transient_for (GTK_WINDOW (preferences_dialog))) {
        gtk_window_set_transient_for (GTK_WINDOW (preferences_dialog),
                                      GTK_WINDOW (parent));
    }

    gtk_window_present (GTK_WINDOW (preferences_dialog));
}

static void
books_preferences_dialog_dispose (GObject *object)
{
    G_OBJECT_CLASS (books_preferences_dialog_parent_class)->dispose (object);
}

static void
books_preferences_dialog_class_init (BooksPreferencesDialogClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = books_preferences_dialog_dispose;

    g_type_class_add_private (klass, sizeof(BooksPreferencesDialogPrivate));
}

static void
response_handler (GtkDialog *dialog,
                  gint res_id)
{
    gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
on_publisher_button_toggled (GtkToggleButton *button,
                             GSettings *settings)
{
    if (gtk_toggle_button_get_active (button))
        g_settings_set_enum (settings, "style-sheet", BOOKS_STYLE_SHEET_PUBLISHER);
}

static void
on_books_button_toggled (GtkToggleButton *button,
                         GSettings *settings)
{
    if (gtk_toggle_button_get_active (button))
        g_settings_set_enum (settings, "style-sheet", BOOKS_STYLE_SHEET_BOOKS);
}

static void
books_preferences_dialog_init (BooksPreferencesDialog *dialog)
{
    BooksPreferencesDialogPrivate *priv;
    GtkBuilder *builder;
    GtkToggleButton *publisher_button;
    GtkToggleButton *books_button;
    GError *error = NULL;

    static gchar *objects[] = {
        "notebook",
        NULL
    };

    dialog->priv = priv = BOOKS_PREFERENCES_DIALOG_GET_PRIVATE (dialog);

    priv->settings = g_settings_new ("com.github.matze.books");

    gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                            GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                            NULL);

    gtk_window_set_title (GTK_WINDOW (dialog), _("Books Preferences"));
    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

    gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
    gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 2);
    gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_action_area (GTK_DIALOG (dialog))), 5);
    gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_action_area (GTK_DIALOG (dialog))), 6);
    
    g_signal_connect (dialog,
                      "response",
                      G_CALLBACK (response_handler),
                      NULL);

    builder = gtk_builder_new ();
    gtk_builder_add_objects_from_resource (builder,
                                           "/com/github/matze/books/ui/books-preferences-dialog.ui",
                                           objects,
                                           &error);
    g_assert_no_error (error);

    priv->notebook = GTK_WIDGET (gtk_builder_get_object (builder, "notebook"));
    g_object_ref (priv->notebook);

    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                        priv->notebook, FALSE, FALSE, 0);
    g_object_unref (priv->notebook);
    gtk_container_set_border_width (GTK_CONTAINER (priv->notebook), 5);

    publisher_button = GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "publisher-style-button"));
    books_button = GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "books-style-button"));

    if (g_settings_get_enum (priv->settings, "style-sheet") == BOOKS_STYLE_SHEET_BOOKS)
        gtk_toggle_button_set_active (books_button, TRUE);
    else
        gtk_toggle_button_set_active (publisher_button, TRUE);

    g_signal_connect (publisher_button,
                      "toggled",
                      G_CALLBACK (on_publisher_button_toggled),
                      priv->settings);

    g_signal_connect (books_button,
                      "toggled",
                      G_CALLBACK (on_books_button_toggled),
                      priv->settings);

    g_object_unref (builder);
}

