# Duke3D Makefile for GNU Make

SRC=source/
OBJ=obj/
EROOT=../build/
EINC=$(EROOT)include/
EOBJ=eobj/
INC=$(SRC)
o=o

ENGINELIB=libengine.a
EDITORLIB=libbuild.a

# debugging enabled
debug=-ggdb
# debugging disabled
#debug=-ggdb -fomit-frame-pointer


DXROOT=c:/sdks/msc/dx61
#FMODROOT=c:/mingw32/fmodapi360win32/api

ENGINEOPTS=-DSUPERBUILD -DPOLYMOST -DUSE_OPENGL -DDYNAMIC_OPENGL

CC=gcc
# -Werror-implicit-function-declaration
CFLAGS=$(debug) -W -Wall -Werror-implicit-function-declaration \
	-Wno-char-subscripts -Wno-unused \
	-march=pentium -funsigned-char -Dmain=app_main -DNO_GCC_BUILTINS \
	-I$(INC) -I$(EINC) -I$(SRC)jmact -I$(SRC)jaudiolib -I../jfaud/inc \
	$(ENGINEOPTS) -DNOCOPYPROTECT \
	-DUSE_GCC_PRAGMAS
LIBS=-lm -L../jfaud -ljfaud
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

uname=$(strip $(shell uname -s))
ifeq ($(findstring Linux,$(uname)),Linux)
	PLATFORM=LINUX
	RENDERTYPE=SDL
	NASMFLAGS+= -f elf
	LIBS+= -lGL
else
	ifeq ($(findstring MINGW32,$(uname)),MINGW32)
		PLATFORM=WINDOWS
		EXESUFFIX=.exe
		CFLAGS+= -DUNDERSCORES -I$(DXROOT)/include
		LIBS+= -lmingwex -lwinmm -mwindows -ldxguid -L$(DXROOT)/lib -lwsock32 -lopengl32
		NASMFLAGS+= -DUNDERSCORES
		GAMEOBJS+= $(OBJ)gameres.$o $(OBJ)winbits.$o
		EDITOROBJS+= $(OBJ)buildres.$o
		RENDERTYPE ?= WIN
		NASMFLAGS+= -f win32
	else
		PLATFORM=UNKNOWN
	endif
endif
	
ifeq ($(RENDERTYPE),SDL)
	#GAMEOBJS+= $(EOBJ)sdlayer.$o
	#EDITOROBJS+= $(EOBJ)sdlayer.$o

	override CFLAGS+= $(subst -Dmain=SDL_main,,$(shell sdl-config --cflags))
	LIBS+= $(shell sdl-config --libs)
	AUDIOLIBOBJ=$(AUDIOLIB_MUSIC_STUB) $(AUDIOLIB_FX_STUB)
else
	ifeq ($(RENDERTYPE),WIN)
		#GAMEOBJS+= $(EOBJ)winlayer.$o
		#EDITOROBJS+= $(EOBJ)winlayer.$o
		
		AUDIOLIBOBJ=$(AUDIOLIB_MUSIC) $(AUDIOLIB_FX)
	endif
endif

GAMEOBJS+= $(AUDIOLIBOBJ)
CFLAGS+= -D$(PLATFORM) -DRENDERTYPE$(RENDERTYPE)=1

.PHONY: clean all engine $(EOBJ)$(ENGINELIB) $(EOBJ)$(EDITORLIB)

# TARGETS
all: duke3d$(EXESUFFIX) build$(EXESUFFIX)

duke3d$(EXESUFFIX): $(GAMEOBJS) $(EOBJ)$(ENGINELIB)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	-rm duke3d.sym$(EXESUFFIX)
	cp duke3d$(EXESUFFIX) duke3d.sym$(EXESUFFIX)
	strip duke3d$(EXESUFFIX)
	
build$(EXESUFFIX): $(EDITOROBJS) $(EOBJ)$(EDITORLIB) $(EOBJ)$(ENGINELIB)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	-rm build.sym$(EXESUFFIX)
	cp build$(EXESUFFIX) build.sym$(EXESUFFIX)
	strip build$(EXESUFFIX)

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


# PHONIES	
clean:
	-rm -f $(OBJ)* duke3d$(EXESUFFIX) duke3d.sym$(EXESUFFIX) build$(EXESUFFIX) build.sym$(EXESUFFIX) core*
	
veryclean: clean
	-rm -f $(EOBJ)*
