#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>

#include "books-removed-dialog.h"


G_DEFINE_TYPE(BooksRemovedDialog, books_removed_dialog, GTK_TYPE_DIALOG)

#define BOOKS_REMOVED_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), BOOKS_TYPE_REMOVED_DIALOG, BooksRemovedDialogPrivate))

struct _BooksRemovedDialogPrivate {
    GtkTreeModel *model;
    GtkTreeView  *view;
};

GtkDialog *
books_removed_dialog_new (GtkTreeModel *model)
{
    BooksRemovedDialog *dialog;
    BooksRemovedDialogPrivate *priv;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    dialog = BOOKS_REMOVED_DIALOG (g_object_new (BOOKS_TYPE_REMOVED_DIALOG, NULL));
    priv = dialog->priv;
    priv->model = model;
    g_object_ref (model);

    gtk_tree_view_set_model (priv->view, priv->model);
    renderer = gtk_cell_renderer_text_new ();

    g_object_set (renderer,
                  "ellipsize-set", TRUE,
                  "ellipsize", PANGO_ELLIPSIZE_START,
                  NULL);

    column = gtk_tree_view_column_new_with_attributes (_("Book"), renderer,
                                                       "text", 0,
                                                       NULL);
    gtk_tree_view_append_column (priv->view, column);

    return GTK_DIALOG (dialog);
}

static void
books_removed_dialog_dispose (GObject *object)
{
    BooksRemovedDialogPrivate *priv;

    priv = BOOKS_REMOVED_DIALOG_GET_PRIVATE (object);
    g_object_unref (priv->model);

    G_OBJECT_CLASS (books_removed_dialog_parent_class)->dispose (object);
}

static void
books_removed_dialog_class_init (BooksRemovedDialogClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = books_removed_dialog_dispose;

    g_type_class_add_private (klass, sizeof(BooksRemovedDialogPrivate));
}

static void
response_handler (GtkDialog *dialog,
                  gint res_id)
{
    gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
books_removed_dialog_init (BooksRemovedDialog *dialog)
{
    BooksRemovedDialogPrivate *priv;
    GtkWidget *content_area;
    GtkWidget *label;

    dialog->priv = priv = BOOKS_REMOVED_DIALOG_GET_PRIVATE (dialog);

    priv->model = NULL;
    priv->view = GTK_TREE_VIEW (gtk_tree_view_new ());

    gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                            GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                            NULL);

    gtk_window_set_title (GTK_WINDOW (dialog), _("Books Removed"));
    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

    gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
    gtk_box_set_spacing (GTK_BOX (content_area), 2);
    gtk_container_set_border_width (GTK_CONTAINER (content_area), 5);
    gtk_box_set_spacing (GTK_BOX (content_area), 6);

    g_signal_connect (dialog,
                      "response",
                      G_CALLBACK (response_handler),
                      NULL);

    label = gtk_label_new (_("The following books could not be found and are removed from your collection:"));
    gtk_widget_set_halign (label, GTK_ALIGN_START);

    gtk_box_pack_start (GTK_BOX (content_area),
                        label, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (content_area),
                        GTK_WIDGET (priv->view), TRUE, TRUE, 0);

    gtk_widget_show (GTK_WIDGET (label));
    gtk_widget_show (GTK_WIDGET (priv->view));
}

