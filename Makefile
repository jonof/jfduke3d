# Duke3D Makefile for GNU Make

SRC=source/
RSRC=rsrc/
OBJ=obj/
EROOT=../build/
EINC=$(EROOT)include/
EOBJ=eobj/
INC=$(SRC)
o=o

# debugging enabled
debug=-ggdb
# debugging disabled
#debug=-ggdb -fomit-frame-pointer


DXROOT=c:/sdks/msc/dx61
#FMODROOT=c:/mingw32/fmodapi360win32/api

ENGINEOPTS=-DSUPERBUILD -DPOLYMOST -DUSE_OPENGL -DDYNAMIC_OPENGL

# This is the list of flags -O1 enables, but actually enabling -O1 causes breakage
OPTIMISE=-fdefer-pop -fmerge-constants -fthread-jumps -floop-optimize -fcrossjumping \
	-fif-conversion -fif-conversion2 -fguess-branch-probability -fcprop-registers

CC=gcc
CFLAGS=$(debug) -W -Wall -Wimplicit $(OPTIMISE) \
	-Wno-char-subscripts -Wno-unused \
	-march=pentium -funsigned-char -fno-strict-aliasing -DNO_GCC_BUILTINS \
	-I$(INC:/=) -I$(EINC:/=) -I$(SRC)jmact -I$(SRC)jaudiolib -I../jfaud/inc \
	$(ENGINEOPTS) -DNOCOPYPROTECT \
	-DUSE_GCC_PRAGMAS
LIBS=-lm #../jfaud/jfaud.a # -L../jfaud -ljfaud
NASMFLAGS=-s #-g
EXESUFFIX=

JMACTOBJ=$(OBJ)util_lib.$o \
	$(OBJ)file_lib.$o \
	$(OBJ)control.$o \
	$(OBJ)keyboard.$o \
	$(OBJ)mouse.$o \
	$(OBJ)mathutil.$o \
	$(OBJ)scriplib.$o

AUDIOLIB_FX_STUB=$(OBJ)audiolib_fxstub.$o
AUDIOLIB_MUSIC_STUB=$(OBJ)audiolib_musicstub.$o
#AUDIOLIB_FX=$(OBJ)audiolib_fx_fmod.$o
AUDIOLIB_FX=$(OBJ)mv_mix.$o \
	  $(OBJ)mv_mix16.$o \
	  $(OBJ)mvreverb.$o \
	  $(OBJ)pitch.$o \
	  $(OBJ)multivoc.$o \
	  $(OBJ)ll_man.$o \
	  $(OBJ)fx_man.$o \
	  $(OBJ)dsoundout.$o
AUDIOLIB_MUSIC=$(OBJ)midi.$o \
	  $(OBJ)mpu401.$o \
	  $(OBJ)music.$o

GAMEOBJS=$(OBJ)game.$o \
	$(OBJ)actors.$o \
	$(OBJ)gamedef.$o \
	$(OBJ)global.$o \
	$(OBJ)menues.$o \
	$(OBJ)player.$o \
	$(OBJ)premap.$o \
	$(OBJ)sector.$o \
	$(OBJ)sounds.$o \
	$(OBJ)rts.$o \
	$(OBJ)config.$o \
	$(OBJ)animlib.$o \
	$(OBJ)testcd.$o \
	$(OBJ)osdfuncs.$o \
	$(OBJ)osdcmds.$o \
	$(JMACTOBJ)

EDITOROBJS=$(OBJ)astub.$o

include $(EROOT)Makefile.shared

ifeq ($(PLATFORM),LINUX)
	NASMFLAGS+= -f elf
endif
ifeq ($(PLATFORM),WINDOWS)
	override CFLAGS+= -DUNDERSCORES -I$(DXROOT)/include
	NASMFLAGS+= -DUNDERSCORES -f win32
	GAMEOBJS+= $(OBJ)gameres.$o $(OBJ)winbits.$o
	EDITOROBJS+= $(OBJ)buildres.$o
endif

ifeq ($(RENDERTYPE),SDL)
	override CFLAGS+= $(subst -Dmain=SDL_main,,$(shell sdl-config --cflags))
	AUDIOLIBOBJ=$(AUDIOLIB_MUSIC_STUB) $(AUDIOLIB_FX_STUB)

	ifeq (1,$(HAVE_GTK2))
		override CFLAGS+= -DHAVE_GTK2 $(shell pkg-config --cflags gtk+-2.0)
		GAMEOBJS+= $(OBJ)game_banner.$o
		EDITOROBJS+= $(OBJ)editor_banner.$o
	endif

	GAMEOBJS+= $(OBJ)game_icon.$o
	EDITOROBJS+= $(OBJ)build_icon.$o
endif
ifeq ($(RENDERTYPE),WIN)
	AUDIOLIBOBJ=$(AUDIOLIB_MUSIC) $(AUDIOLIB_FX)
endif

GAMEOBJS+= $(AUDIOLIBOBJ)

.PHONY: clean all engine $(EOBJ)$(ENGINELIB) $(EOBJ)$(EDITORLIB)

# TARGETS
all: duke3d$(EXESUFFIX) build$(EXESUFFIX)

duke3d$(EXESUFFIX): $(GAMEOBJS) $(EOBJ)$(ENGINELIB)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) -Wl,-Map=$@.map
#	-rm duke3d.sym$(EXESUFFIX)
#	cp duke3d$(EXESUFFIX) duke3d.sym$(EXESUFFIX)
#	strip duke3d$(EXESUFFIX)
	
build$(EXESUFFIX): $(EDITOROBJS) $(EOBJ)$(EDITORLIB) $(EOBJ)$(ENGINELIB)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) -Wl,-Map=$@.map
#	-rm build.sym$(EXESUFFIX)
#	cp build$(EXESUFFIX) build.sym$(EXESUFFIX)
#	strip build$(EXESUFFIX)

include Makefile.deps

$(EOBJ)$(ENGINELIB):
	-mkdir $(EOBJ)
	$(MAKE) -C $(EROOT) "OBJ=$(CURDIR)/$(EOBJ)" "CFLAGS=$(ENGINEOPTS)" enginelib

$(EOBJ)$(EDITORLIB):
	-mkdir $(EOBJ)
	$(MAKE) -C $(EROOT) "OBJ=$(CURDIR)/$(EOBJ)" "CFLAGS=$(ENGINEOPTS)" editorlib

# RULES
$(OBJ)%.$o: $(SRC)%.nasm
	nasm $(NASMFLAGS) $< -o $@
$(OBJ)%.$o: $(SRC)jaudiolib/%.nasm
	nasm $(NASMFLAGS) $< -o $@

$(OBJ)%.$o: $(SRC)%.c
	$(CC) $(CFLAGS) -c $< -o $@ 2>&1
$(OBJ)%.$o: $(SRC)jmact/%.c
	$(CC) $(CFLAGS) -c $< -o $@ 2>&1
$(OBJ)%.$o: $(SRC)jaudiolib/%.c
	$(CC) $(CFLAGS) -c $< -o $@ 2>&1

$(OBJ)%.$o: $(SRC)misc/%.rc
	windres -i $^ -o $@

$(OBJ)%.$o: $(SRC)util/%.c
	$(CC) $(CFLAGS) -c $< -o $@ 2>&1

$(OBJ)%.$o: $(RSRC)%.c
	$(CC) $(CFLAGS) -c $< -o $@ 2>&1

$(OBJ)game_banner.$o: $(RSRC)game_banner.c
$(OBJ)editor_banner.$o: $(RSRC)editor_banner.c
$(RSRC)game_banner.c: $(RSRC)game.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@
$(RSRC)editor_banner.c: $(RSRC)build.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@

# PHONIES	
clean:
	-rm -f $(OBJ)* duke3d$(EXESUFFIX) duke3d.sym$(EXESUFFIX) build$(EXESUFFIX) build.sym$(EXESUFFIX) core*
	
veryclean: clean
	-rm -f $(EOBJ)*
