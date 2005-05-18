SUBDIRS = src data

EXTRA_DIST = matchbox-stroke.desktop matchbox-stroke.png

desktopdir = $(datadir)/applications/inputmethods
desktop_DATA = matchbox-stroke.desktop

pixmapsdir = $(datadir)/pixmaps/
pixmaps_DATA = matchbox-stroke.png

snapshot:
	$(MAKE) dist distdir=$(PACKAGE)-snap`date +"%Y%m%d"`
