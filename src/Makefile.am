## Process this file with automake to produce Makefile.in

bin_PROGRAMS = gimp-plugin-insanebump

bindir = $(GIMP_LIBDIR)/plug-ins/gimp-plugin-insanebump

gimp_plugin_insanebump_SOURCES =        \
	plugin-intl.h                   \
	interface.c                     \
	interface.h                     \
	main.c                          \
	main.h                          \
	InsaneBump.c                    \
	InsaneBump.h                    \
	normalmap.c                     \
	PluginConnectors.c              \
	PluginConnectors.h              \
	scale.c                         \
	scale.h                         \
	objects/cube.h                  \
	objects/quad.h                  \
	objects/sphere.h                \
	objects/teapot.h                \
	objects/torus.h                 \
	preview_area.c

AM_CPPFLAGS =                           \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"  \
	-DDATADIR=\""$(DATADIR)"\"      \
	-I$(top_srcdir)                 \
	$(GIMP_CFLAGS)

LDADD = $(GIMP_LIBS)
