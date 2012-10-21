AC_PREREQ(2.60)
AC_INIT([books],
        [0.1],
        [matthias.vogelgesang@gmail.com])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([configure.ac])

AM_INIT_AUTOMAKE([1.11 -Wall -Werror dist-bzip2 -Wno-portability])
AM_SILENT_RULES([yes])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources)

PKG_CHECK_MODULES([BOOKS], 
            [gtk+-3.0
             webkitgtk-3.0
             libarchive
             libxml-2.0
             sqlite3 >= 3.3])

AC_PREFIX_DEFAULT("/usr")

AC_CONFIG_FILES([
            Makefile
            src/Makefile])
AC_OUTPUT
