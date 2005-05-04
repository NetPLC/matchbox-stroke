PREFIXDIR  = $(prefix)
PKGDATADIR = $(datadir)/matchbox-stroke
DATADIR    = $(datadir)

INCLUDES = -DDATADIR=\"$(DATADIR)\" -DPKGDATADIR=\"$(PKGDATADIR)\" -DPREFIX=\"$(PREFIXDIR)\" $(MBSTROKE_CFLAGS) $(EXPAT_CFLAGS)

bin_PROGRAMS = matchbox-stroke

matchbox_stroke_LDADD = $(MBSTROKE_LIBS) $(EXPAT_LIBS) -lm

matchbox_stroke_SOURCES =                                          \
       matchbox-stroke.h                                           \
       matchbox-stroke.c                                           \
       matchbox-stroke-ui.c                                        \
       matchbox-stroke-recog.c                                     \
       util-hash.c                                                 \
       util.c


