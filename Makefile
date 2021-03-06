#
# Makefile for a Video Disk Recorder plugin
#
# $Id$

# The official name of this plugin.
# This name will be used in the '-P...' option of VDR to load the plugin.
# By default the main source file also carries this name.
# IMPORTANT: the presence of this macro is important for the Make.config
# file. So it must be defined, even if it is not used here!
#
PLUGIN = cdplayer

### The version number of this plugin (taken from the main source file):

VERSION = $(shell grep 'static const char \*VERSION *=' $(PLUGIN).h | awk '{ print $$6 }' | sed -e 's/[";]//g')

### The directory environment:

# Use package data if installed...otherwise assume we're under the VDR source directory:
PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell pkg-config --variable=$(1) vdr || pkg-config --variable=$(1) ../../../vdr.pc))
LIBDIR = $(call PKGCFG,libdir)
LOCDIR = $(call PKGCFG,locdir)
CFGDIR = $(call PKGCFG,configdir)/plugins/$(PLUGIN)
PLGCFG = $(call PKGCFG,plgcfg)
#
TMPDIR = /tmp

### Uncomment if you use graphtft
#DEFINES += -DUSE_GRAPHTFT

export CFLAGS   = $(call PKGCFG,cflags)
export CXXFLAGS = $(call PKGCFG,cxxflags)

ifneq (exists, $(shell pkg-config libcdio && echo exists))
  $(warning ******************************************************************)
  $(warning 'libcdio' not detected!)
  $(warning ******************************************************************)
else
  USE_CDIO=1
endif

ifneq (exists, $(shell pkg-config libcdio_cdda && echo exists))
  $(warning ******************************************************************)
  $(warning 'libcdio_cdda' not detected! ')
  $(warning ******************************************************************)
else
  USE_CDIOCDDA=1
endif

ifneq (exists, $(shell pkg-config libcddb && echo exists))
  $(warning ******************************************************************)
  $(warning 'libcddb' not detected! ')
  $(warning ******************************************************************)
endif

ifneq (exists, $(shell pkg-config libcdio_paranoia && echo exists))
  $(warning ******************************************************************)
  $(warning 'libcdio_paranoia' not detected! ')
  $(warning 'compiling without paranoia support ')
  $(warning ******************************************************************)
else 
### Comment out if you don't like libparanoia support
ifdef NOPARANOIA
  $(warning 'paranoia support disabled')
else
  USE_PARANOIA=1
  DEFINES += -DUSE_PARANOIA=1
endif
endif
### The version number of VDR's plugin API (taken from VDR's "config.h"):

APIVERSION = $(call PKGCFG,apiversion)

### Allow user defined options to overwrite defaults:

-include $(PLGCFG)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:

SOFILE = libvdr-$(PLUGIN).so

 ### Includes and Defines (add further entries here):
 
INCLUDES +=
DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

### The object files (add further files here):

OBJS = $(PLUGIN).o cd_control.o pes_audio_converter.o bufferedcdio.o \
				   cdioringbuf.o cdinfo.o cdmenu.o

ifdef USE_CDIO
LIBS += $(shell pkg-config --libs libcdio)
endif

ifdef USE_CDIOCDDA
LIBS += $(shell pkg-config --libs libcdio_cdda)
endif

ifdef USE_PARANOIA
LIBS += $(shell pkg-config --libs libcdio_paranoia)
endif

LIBS += $(shell pkg-config --libs libcddb)

### The main target:

all: $(SOFILE) i18n

### Implicit rules:

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(CEXTRA) -c $(DEFINES) $(INCLUDES) $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cc) *.h > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR     = po
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmo    = $(addsuffix .mo, $(foreach file, $(I18Npo), $(basename $(file))))
I18Nmsgs  = $(addprefix $(DESTDIR)$(LOCDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -v -c -o $@ $<

$(I18Npot): $(wildcard *.cc *.h)
	xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) --msgid-bugs-address='<see README>' -o $@ `ls $^`

%.po: $(I18Npot)
	msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(DESTDIR)$(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	install -D -m644 $< $@

.PHONY: i18n
i18n: $(I18Nmo) $(I18Npot)

install-i18n: $(I18Nmsgs)

### Targets:

$(SOFILE): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) $(LIBS) -o $@

install-lib: $(SOFILE)
	install -D $^ $(DESTDIR)$(LIBDIR)/$^.$(APIVERSION)

install-contrib: 
	@mkdir -p $(DESTDIR)$(CFGDIR)
	@cp -n contrib/*.mpg $(DESTDIR)$(CFGDIR)

install: install-lib install-i18n install-contrib

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz  --exclude=*/push.sh -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~
