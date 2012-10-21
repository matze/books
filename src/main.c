#include <gtk/gtk.h>

#include "books-main-window.h"


int
main (int argc,
      char *argv[])
{
    GtkWidget *window;
    GError *error = NULL;

    gtk_init (&argc, &argv);

    window = books_main_window_new ();
    gtk_widget_set_size_request (window, 800, 600);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);

    g_signal_connect (G_OBJECT (window), "delete-event",
                      G_CALLBACK (gtk_main_quit), NULL);

    gtk_widget_show_all (window);
    gtk_main ();

    return 0;
}
