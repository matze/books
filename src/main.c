#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <locale.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "books-main-window.h"


int
main (int argc,
      char *argv[])
{
    GtkWidget *window;
    gchar *locale_dir;

    gtk_init (&argc, &argv);

    locale_dir = g_build_filename (DATADIR,
                                   "locale",
                                   NULL);
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, locale_dir);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    window = books_main_window_new ();
    gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);

    g_signal_connect (G_OBJECT (window), "delete-event",
                      G_CALLBACK (gtk_main_quit), NULL);

    gtk_widget_show (window);
    gtk_main ();

    g_free (locale_dir);
    return 0;
}
