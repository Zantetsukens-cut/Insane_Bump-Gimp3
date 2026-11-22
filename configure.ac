dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

dnl Name your plug-in here
m4_define([plugin_name], [Gimp3_plugin_InsaneBump])

dnl These three define the plug-in version number
m4_define([plugin_major_version], [3])
m4_define([plugin_minor_version], [0])
m4_define([plugin_micro_version], [0])

m4_define([plugin_version],
          [plugin_major_version.plugin_minor_version.plugin_micro_version-gimp3])

AC_INIT([InsaneBump], [plugin_version])

AC_DEFINE(PLUGIN_NAME, PACKAGE_NAME, [Plug-In name])
AC_DEFINE(PLUGIN_VERSION, PACKAGE_VERSION, [Plug-In version])

AC_DEFINE(PLUGIN_MAJOR_VERSION, plugin_major_version, [Plug-In major version])
AC_DEFINE(PLUGIN_MINOR_VERSION, plugin_minor_version, [Plug-In minor version])
AC_DEFINE(PLUGIN_MICRO_VERSION, plugin_micro_version, [Plug-In micro version])

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([no-define])

AC_PROG_CC
AC_HEADER_STDC

ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

AM_MAINTAINER_MODE

dnl Use -Wall if we have gcc.
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl

GIMP_REQUIRED_VERSION=3.0.0
GTK_REQUIRED_VERSION=3.24.0
GEGL_REQUIRED_VERSION=0.4.0

PKG_CHECK_MODULES(GIMP, [
  gimp-3.0 >= $GIMP_REQUIRED_VERSION
  gimpui-3.0 >= $GIMP_REQUIRED_VERSION
  gtk+-3.0 >= $GTK_REQUIRED_VERSION
  gegl-0.4 >= $GEGL_REQUIRED_VERSION
])

AC_SUBST(GIMP_CFLAGS)
AC_SUBST(GIMP_LIBS)

GIMP_LIBDIR=`$PKG_CONFIG --variable=gimplibdir gimp-3.0`
AC_SUBST(GIMP_LIBDIR)

dnl i18n stuff
GETTEXT_PACKAGE=gimp30-plugin-insanebump
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
                   [The gettext translation domain.])

dnl Add the languages which your application supports here.
ALL_LINGUAS="az de fr sk sv zh_TW"

AC_PROG_INTLTOOL
AM_GLIB_GNU_GETTEXT

AC_CHECK_FUNCS(bind_textdomain_codeset)

LOCALEDIR='${datadir}/locale'
DATADIR='${datadir}/plugin_name'

AC_SUBST(LOCALEDIR)
AC_SUBST(DATADIR)

AC_CONFIG_FILES([
Makefile
src/Makefile
po/Makefile.in
help/Makefile
help/en/Makefile
help/images/Makefile
])

AC_OUTPUT
