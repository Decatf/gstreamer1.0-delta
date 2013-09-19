# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstdeltadsp.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgstdeltadsp_la_SOURCES = gstdeltadsp.c gstdeltadsp.h delta.c delta.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstdeltadsp_la_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) $(GST_BASE_CFLAGS) $(GST_CFLAGS)
libgstdeltadsp_la_LIBADD = $(GST_PLUGINS_BASE_LIBS) \
	-lgstaudio-$(GST_API_VERSION) $(GST_BASE_LIBS) \
	$(GST_LIBS) $(LIBM)
libgstdeltadsp_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstdeltadsp_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstdeltadsp.h delta.h

