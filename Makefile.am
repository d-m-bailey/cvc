AUTOMAKE_OPTIONS = foreign
#SUBDIRS = po src
SUBDIRS = src scripts src_py doc

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = LICENSE README.md config.rpath \
	src_py scripts doc \
	src/LICENSE src/README.md src/ChangeLog
dist-hook:
	rm -rf $(distdir)/src_py/Makefile
	rm -rf $(distdir)/scripts/Makefile
	rm -rf $(distdir)/doc/Makefile
