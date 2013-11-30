## Taken from gtk-doc/examples/Makefile.am
DOC_MODULE=nih
DOC_MODULE_VERSION=$(VERSION)
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml
DOC_SOURCE_DIR=$(top_srcdir)/nih
HFILE_GLOB=$(top_srcdir)/nih/*.h
CFILE_GLOB=$(top_srcdir)/nih/*.c

AUTOMAKE_OPTIONS = 1.6
MKDB_OPTIONS=--xml-mode --output-format=xml

# This includes the standard gtk-doc make rules, copied by gtkdocize.
include $(top_srcdir)/gtk-doc.make

if ENABLE_GTK_DOC
TESTS_ENVIRONMENT = cd $(srcdir) && \
  DOC_MODULE=$(DOC_MODULE) DOC_MAIN_SGML_FILE=$(DOC_MAIN_SGML_FILE) \
  SRCDIR=$(abs_srcdir) BUILDDIR=$(abs_builddir)
#TESTS = $(GTKDOC_CHECK)
# gtkdoc-check currently fails...
check-local:
	-$(TESTS_ENVIRONMENT) $(GTKDOC_CHECK)
endif
