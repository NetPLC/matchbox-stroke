SUBDIRS = src data

EXTRA_DIST = matchbox-stroke.desktop matchbox-stroke.png \
             matchbox-stroke-fullscreen.desktop matchbox-stroke-fullscreen.png

desktopdir = $(datadir)/applications/inputmethods
desktop_DATA = matchbox-stroke.desktop 

# disabled for now;
# matchbox-stroke-fullscreen.desktop 

pixmapsdir = $(datadir)/pixmaps/
pixmaps_DATA = matchbox-stroke.png matchbox-stroke-fullscreen.png

snapshot:
	$(MAKE) dist distdir=$(PACKAGE)-snap`date +"%Y%m%d"`
