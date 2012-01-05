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

### The C++ compiler and options:

CXX      ?= g++
CXXFLAGS ?= -g -O2 -Wall -Woverloaded-virtual -Wno-parentheses 
CXXFLAGS += -Wextra -pedantic

### The directory environment:

VDRDIR = ../../..
LIBDIR = ../../lib
TMPDIR = /tmp

### Uncomment if you use graphtft
#DEFINES += -DUSE_GRAPHTFT

### Make sure that necessary options are included:

-include $(VDRDIR)/Make.global

### Allow user defined options to overwrite defaults:

-include $(VDRDIR)/Make.config

ifneq (exists, $(shell pkg-config libcdio_cdda && echo exists))
  $(warning ******************************************************************)
  $(warning 'libcdio_cdda' not detected! ')
  $(warning ******************************************************************)
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

APIVERSION = $(shell sed -ne '/define APIVERSION/s/^.*"\(.*\)".*$$/\1/p' $(VDRDIR)/config.h)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### Includes and Defines (add further entries here):

INCLUDES += -I$(VDRDIR)/include

DEFINES += -D_GNU_SOURCE -DPLUGIN_NAME_I18N='"$(PLUGIN)"'

### The object files (add further files here):

OBJS = $(PLUGIN).o cd_control.o pes_audio_converter.o bufferedcdio.o \
				   cdioringbuf.o cdinfo.o cdmenu.o
LIBS = $(shell pkg-config --libs libcddb)  
LIBS += $(shell pkg-config --libs libcdio_cdda)
ifdef USE_PARANOIA
LIBS += $(shell pkg-config --libs libcdio_paranoia)
endif
### The main target:

all: libvdr-$(PLUGIN).so i18n

### Implicit rules:

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cc) *.h > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR     = po
LOCALEDIR = $(VDRDIR)/locale
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmsgs  = $(addprefix $(LOCALEDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	msgfmt -v -c -o $@ $<

$(I18Npot): $(wildcard *.cc *.h)
	xgettext -C -cTRANSLATORS --no-wrap -n -i -F --package-name="cdplayer plugin" -k -ktr -ktrNOOP --msgid-bugs-address='vdr@uli-eckhardt.de' -o $@ $^

%.po: $(I18Npot)
	msgmerge -v -U --no-wrap --backup=none -q $@ $<
	@touch $@

$(I18Nmsgs): $(LOCALEDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	@mkdir -p $(dir $@)
	cp $< $@

.PHONY: i18n
i18n: $(I18Nmsgs) $(I18Npot)

### Targets:

libvdr-$(PLUGIN).so: $(OBJS)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) $(LIBS) -o $@
	@cp --remove-destination $@ $(LIBDIR)/$@.$(APIVERSION)

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(OBJS) $(DEPFILE) *.so *.tgz core* *~ $(PODIR)/*.mo $(PODIR)/*.pot
