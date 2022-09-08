# Blood Makefile for GNU Make

# Create Makefile.user yourself to provide your own overrides
# for configurable values
-include Makefile.user

##
##
## CONFIGURABLE OPTIONS
##
##

# Debugging options
RELEASE ?= 1

# Engine source code path
EROOT ?= jfbuild

# JMACT library source path
MACTROOT ?= jfmact

# JFAudioLib source path
AUDIOLIBROOT ?= jfaudiolib

# ENet source path
ENETROOT ?= enet

# libsmackerdec source path
SMACKERROOT ?= libsmacker

# Engine options
#  USE_POLYMOST   - enables Polymost renderer
#  USE_OPENGL     - enables OpenGL support in Polymost
#     Define as 0 to disable OpenGL
#     Define as USE_GL2 (or 1 or 2) for GL 2.0/2.1 profile
#     Define as USE_GLES2 (or 12) for GLES 2.0 profile
#  USE_ASM        - enables the use of assembly code
USE_POLYMOST ?= 0
USE_OPENGL ?= 0
USE_ASM ?= 0


##
##
## HERE BE DRAGONS
##
##

# build locations
SRC=src
RSRC=rsrc
EINC=$(EROOT)/include
ELIB=$(EROOT)
INC=$(SRC)
o=o
res=o

ifneq (0,$(RELEASE))
  # debugging disabled
  debug=-fomit-frame-pointer -O2
else
  # debugging enabled
  debug=-ggdb -Og -D_DEBUG #-Werror
endif

include $(AUDIOLIBROOT)/Makefile.shared

CC?=gcc
CXX?=g++
NASM?=nasm
RC?=windres
OURCFLAGS=$(debug) -W -Wall -Wno-write-strings -Wno-char-subscripts -Wno-unused \
	-fno-strict-aliasing  -funsigned-char -DNO_GCC_BUILTINS -DENGINE_19960925 -DASS_REVERSESTEREO \
	 -I$(EINC) -I$(MACTROOT) -I$(AUDIOLIBROOT)/include -I$(ENETROOT)/include -I$(SMACKERROOT)

OURCXXFLAGS=-fno-exceptions -fno-rtti -Wno-narrowing -I$(INC)

LIBS=-lm
GAMELIBS=
NASMFLAGS=-s #-g
EXESUFFIX=

JMACTOBJ=$(MACTROOT)/util_lib.$o \
	$(MACTROOT)/file_lib.$o \
	$(MACTROOT)/control.$o \
	$(MACTROOT)/keyboard.$o \
	$(MACTROOT)/mouse.$o \
	$(MACTROOT)/scriplib.$o

ENETOBJ=$(ENETROOT)/callbacks.o \
	$(ENETROOT)/host.o \
	$(ENETROOT)/list.o \
	$(ENETROOT)/packet.o \
	$(ENETROOT)/peer.o \
	$(ENETROOT)/protocol.o \
	$(ENETROOT)/unix.o \
	$(ENETROOT)/win32.o

SMACKEROBJ=$(SMACKERROOT)/smacker.o

GAMEOBJS=$(SRC)/actor.o \
	$(SRC)/ai.o \
	$(SRC)/aibat.o \
	$(SRC)/aibeast.o \
	$(SRC)/aiboneel.o \
	$(SRC)/aiburn.o \
	$(SRC)/aicaleb.o \
	$(SRC)/aicerber.o \
	$(SRC)/aicult.o \
	$(SRC)/aigarg.o \
	$(SRC)/aighost.o \
	$(SRC)/aigilbst.o \
	$(SRC)/aihand.o \
	$(SRC)/aihound.o \
	$(SRC)/aiinnoc.o \
	$(SRC)/aipod.o \
	$(SRC)/airat.o \
	$(SRC)/aispid.o \
	$(SRC)/aitchern.o \
	$(SRC)/aizomba.o \
	$(SRC)/aizombf.o \
	$(SRC)/asound.o \
	$(SRC)/blood.o \
	$(SRC)/callback.o \
	$(SRC)/choke.o \
	$(SRC)/common.o \
	$(SRC)/compat.o \
	$(SRC)/config.o \
	$(SRC)/db.o \
	$(SRC)/demo.o \
	$(SRC)/dude.o \
	$(SRC)/endgame.o \
	$(SRC)/eventq.o \
	$(SRC)/fire.o \
	$(SRC)/fx.o \
	$(SRC)/gamemenu.o \
	$(SRC)/getopt.o \
	$(SRC)/gfx.o \
	$(SRC)/gib.o \
	$(SRC)/globals.o \
	$(SRC)/gui.o \
	$(SRC)/inifile.o \
	$(SRC)/iob.o \
	$(SRC)/levels.o \
	$(SRC)/loadsave.o \
	$(SRC)/map2d.o \
	$(SRC)/messages.o \
	$(SRC)/misc.o \
	$(SRC)/network.o \
	$(SRC)/palette.o \
	$(SRC)/player.o \
	$(SRC)/qav.o \
	$(SRC)/qheap.o \
	$(SRC)/sectorfx.o \
	$(SRC)/seq.o \
	$(SRC)/sfx.o \
	$(SRC)/sound.o \
	$(SRC)/trig.o \
	$(SRC)/triggers.o \
	$(SRC)/warp.o \
	$(SRC)/weapon.o \
	$(SRC)/controls.o \
	$(SRC)/credits.o \
	$(SRC)/gameutil.o \
	$(SRC)/menu.o \
	$(SRC)/mirrors.o \
	$(SRC)/osdcmd.o \
	$(SRC)/replace.o \
	$(SRC)/resource.o \
	$(SRC)/tile.o \
	$(SRC)/view.o \
	$(SRC)/screen.o \
	$(SMACKEROBJ) \
	$(ENETOBJ) \
	$(JMACTOBJ)

# NOONE_EXTENSIONS
#GAMEOBJS+= \
#	$(SRC)/nnexts.o \
#	$(SRC)/aiunicult.o \
#	$(SRC)/barf.o \

include $(EROOT)/Makefile.shared

ifeq ($(PLATFORM),LINUX)
	NASMFLAGS+= -f elf
	GAMELIBS+= $(JFAUDIOLIB_LDFLAGS)
endif
ifeq ($(PLATFORM),BSD)
	NASMFLAGS+= -f elf
	GAMELIBS+= $(JFAUDIOLIB_LDFLAGS) -pthread
endif
ifeq ($(PLATFORM),WINDOWS)
	OURCFLAGS+= -I$(DXROOT)/include
	NASMFLAGS+= -f win32 --prefix _
	GAMEOBJS+= $(RSRC)/gameres.$(res) $(SRC)/winbits.$o $(SRC)/startwin_game.$o
	GAMELIBS+= -ldsound \
	       $(AUDIOLIBROOT)/third-party/mingw32/lib/libvorbisfile.a \
	       $(AUDIOLIBROOT)/third-party/mingw32/lib/libvorbis.a \
	       $(AUDIOLIBROOT)/third-party/mingw32/lib/libogg.a
endif

ifeq ($(RENDERTYPE),SDL)
	OURCFLAGS+= $(SDLCONFIG_CFLAGS)
	LIBS+= $(SDLCONFIG_LIBS)

	ifeq (1,$(HAVE_GTK))
		OURCFLAGS+= $(GTKCONFIG_CFLAGS)
		LIBS+= $(GTKCONFIG_LIBS)
		GAMEOBJS+= $(SRC)/startgtk_game.$o $(RSRC)/startgtk_game_gresource.$o
	endif

	GAMEOBJS+= $(RSRC)/sdlappicon_game.$o
endif

# Source-control version stamping
ifneq (,$(findstring git version,$(shell git --version)))
GAMEOBJS+= $(SRC)/version-auto.$o
else
GAMEOBJS+= $(SRC)/version.$o
endif

OURCFLAGS+= $(BUILDCFLAGS)
LIBS+= $(BUILDLIBS)

.PHONY: clean all engine $(ELIB)/$(ENGINELIB) $(AUDIOLIBROOT)/$(JFAUDIOLIB)

# TARGETS

all: nblood$(EXESUFFIX)

nblood$(EXESUFFIX): $(GAMEOBJS) $(ELIB)/$(ENGINELIB) $(AUDIOLIBROOT)/$(JFAUDIOLIB)
	$(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -o $@ $^ $(LIBS) $(GAMELIBS) -Wl,-Map=$@.map


#include Makefile.deps

.PHONY: enginelib
enginelib:
	$(MAKE) -C $(EROOT) \
		USE_POLYMOST=$(USE_POLYMOST) \
		USE_OPENGL=$(USE_OPENGL) \
		USE_ASM=$(USE_ASM) \
		RELEASE=$(RELEASE) CFLAGS="-DENGINE_19960925" $@
$(EROOT)/generatesdlappicon$(EXESUFFIX):
	$(MAKE) -C $(EROOT) generatesdlappicon$(EXESUFFIX)

$(ELIB)/$(ENGINELIB): enginelib
$(AUDIOLIBROOT)/$(JFAUDIOLIB):
	$(MAKE) -C $(AUDIOLIBROOT) RELEASE=$(RELEASE)

# RULES
$(SRC)/%.$o: $(SRC)/%.nasm
	$(NASM) $(NASMFLAGS) $< -o $@

$(SRC)/%.$o: $(SRC)/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@
$(SRC)/%.$o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@
$(MACTROOT)/%.$o: $(MACTROOT)/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@
$(ENETROOT)/%.$o: $(ENETROOT)/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@
$(SMACKERROOT)/%.$o: $(SMACKERROOT)/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@

$(RSRC)/%.$(res): $(RSRC)/%.rc
	$(RC) -i $< -o $@ --include-dir=$(EINC) --include-dir=$(SRC)

$(RSRC)/%.$o: $(RSRC)/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@

$(RSRC)/%_gresource.c: $(RSRC)/%.gresource.xml
	glib-compile-resources --generate --manual-register --c-name=startgtk --target=$@ --sourcedir=$(RSRC) $<
$(RSRC)/%_gresource.h: $(RSRC)/%.gresource.xml
	glib-compile-resources --generate --manual-register --c-name=startgtk --target=$@ --sourcedir=$(RSRC) $<
$(RSRC)/sdlappicon_%.c: $(RSRC)/%.png $(EROOT)/generatesdlappicon$(EXESUFFIX)
	$(EROOT)/generatesdlappicon$(EXESUFFIX) $< > $@

# PHONIES
clean:
	-rm -f $(GAMEOBJS)
	$(MAKE) -C $(EROOT) clean
	$(MAKE) -C $(AUDIOLIBROOT) clean

veryclean: clean
	-rm -f nblood$(EXESUFFIX) core*
	$(MAKE) -C $(EROOT) veryclean

.PHONY: $(SRC)/version-auto.c
$(SRC)/version-auto.c:
	printf "const char *s_buildRev = \"%s\";\n" "$(shell git describe --always || echo git error)" > $@
	echo "const char *s_buildTimestamp = __DATE__ \" \" __TIME__;" >> $@
