INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/poppler			\
	-I$(top_srcdir)/qt			\
	$(POPPLER_QT4_CXXFLAGS)


poppler_includedir = $(includedir)/poppler

poppler_include_HEADERS =			\
	poppler-qt4.h				\
	../../qt/poppler-page-transition.h

lib_LTLIBRARIES = libpoppler-qt4.la

libpoppler_qt4_la_SOURCES =			\
	poppler-document.cc			\
	poppler-page.cc				\
	poppler-fontinfo.cc			\
	poppler-embeddedfile.cc			\
	poppler-textbox.cc			\
	../../qt/poppler-page-transition.cc	\
	poppler-private.h

libpoppler_qt4_la_LIBADD = 			\
	$(top_builddir)/poppler/libpoppler.la	\
	$(POPPLER_QT4_LIBS)

libpoppler_qt4_la_LDFLAGS = -version-info 1:0:0

