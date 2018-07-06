#
# OpenBOR - http://www.LavaLit.com
# -----------------------------------------------------------------------
# Licensed under the BSD license, see LICENSE in OpenBOR root for details.
#
# Copyright (c) 2004 - 2009 OpenBOR Team
#

#------------------------------------------------------------------------------
#
#               	OpenBOR Makefile for ALL TARGET_PLATFORMs 
#
#                      PSP, Dreamcast, GP2X, Windows & Linux
#
#------------------------------------------------------------------------------

ifndef VERSION_NAME
VERSION_NAME = OpenBOR
endif

#------------------------------------------------------------------------------
# Defines
#------------------------------------------------------------------------------

ifdef BUILD_PSP
TARGET          = $(VERSION_NAME)
EXTRA_TARGETS   = EBOOT.PBP
PBPNAME_STR     = $(TARGET)
TARGET_PLATFORM = "PSP"
BUILDING	= 1
endif


ifdef BUILD_WIN
TARGET          = $(VERSION_NAME).elf
EXTRA_TARGETS   = $(VERSION_NAME).exe
RES             = sdl/$(VERSION_NAME).res
BUILD_SDL       = 1
BUILD_WINLIN    = 1
WIN             = 1
BUILDING        = 1
OBJTYPE         = elf
YASM 	        = $(TOOLS)/yasm-0.7.0-win32.exe
CC              = $(SDKPATH)/bin/gcc.exe
STRIP 	        = $(SDKPATH)/bin/strip.exe
EXTRAS          = $(SDKPATH)/include/SDL
INCLUDES        = $(SDKPATH)/include
LIBRARIES       = $(SDKPATH)/lib
TARGET_PLATFORM = "Windows"
GFX             = 1
ifeq ($(MACHINENAME), i386)
  MMX             = 1
endif
ifeq ($(MACHINENAME), i586)
  MMX             = 1
endif
ifeq ($(MACHINENAME), i686)
  MMX             = 1
endif
endif


ifdef BUILD_LINUX
TARGET 	        = $(VERSION_NAME).elf
EXTRA_TARGETS	= $(VERSION_NAME)
BUILD_SDL       = 1
BUILD_WINLIN    = 1
BUILD_GP2LIN    = 1
LINUX           = 1
BUILDING        = 1
GFX             = 1
YASM 	        = yasm
CC  	        = gcc
STRIP 	        = strip
EXTRAS          = $(LNXDEV)/include/SDL
INCLUDES        = $(LNXDEV)/include
ifeq ($(HOST_PLATFORM), Linux)
  OBJTYPE         = elf
  LIBRARIES       = $(LNXDEV)/local/lib
  TARGET_PLATFORM = "Linux"
endif
ifeq ($(HOST_PLATFORM), Darwin)
  OBJTYPE         = macho
  OSX             = 1
  INCLUDES       += /usr/include/malloc
  LIBRARIES       = $(LNXDEV)/lib
  TARGET_PLATFORM = "Darwin"
endif
ifeq ($(MACHINENAME), i386)
  MMX             = 1
endif
ifeq ($(MACHINENAME), i586)
  MMX             = 1
endif
ifeq ($(MACHINENAME), i686)
  MMX             = 1
endif
endif

ifdef BUILD_GP2X
TARGET 	        = $(VERSION_NAME).elf
EXTRA_TARGETS	= $(VERSION_NAME).gpe
BUILD_SDL       = 1
BUILD_GP2LIN    = 1
GP2X            = 1
BUILDING        = 1
GFX             = 1
TARGET_PLATFORM = "GP2X"
CC              = $(GP2XDEV)/arm-open2x-linux-gcc
STRIP 	        = $(GP2XDEV)/arm-open2x-linux-strip
UNAME          := $(shell uname)
EXTRAS          = $(SDKPATH)/include/SDL
INCLUDES        = $(SDKPATH)/include
LIBRARIES       = $(SDKPATH)/lib
endif


#Added by Shin-NiL
ifdef BUILD_DINGOO
DINGOODEV     	= /opt/mipsel-linux-uclibc/usr/bin
SDKPATH		= /opt/mipsel-linux-uclibc/usr
TARGET 	        = $(VERSION_NAME).elf
EXTRA_TARGETS	= $(VERSION_NAME).dge
BUILD_SDL       = 1
BUILD_GP2LIN    = 1
#GP2X            = 1
DINGOO		= 1
BUILDING        = 1
GFX             = 1
TARGET_PLATFORM = "DINGOO"
CC              = $(DINGOODEV)/mipsel-linux-gcc
STRIP 	        = $(DINGOODEV)/mipsel-linux-strip
#UNAME          := $(shell uname)
EXTRAS          = $(SDKPATH)/include/SDL
INCLUDES        = $(SDKPATH)/include
LIBRARIES       = $(SDKPATH)/lib
endif


ifdef BUILD_DC
TARGET 	        = $(VERSION_NAME).elf
EXTRA_TARGETS   = 1ST_READ.BIN
DC              = 1
BUILDING        = 1
TARGET_PLATFORM = "Dreamcast"
EXTRAS          = $(KOS_BASE)/../kos-ports/include/SDL-1.2.9 \
                  $(KOS_BASE)/../kos-ports/include/png \
                  $(KOS_BASE)/../kos-ports/include/zlib
endif


#------------------------------------------------------------------------------
# Directories
#------------------------------------------------------------------------------

ifdef BUILD_PSP
INCS 	      	= psp
endif


ifdef BUILD_DC
INCS 	      	= $(EXTRAS)                                                   \
                  dc
endif


ifdef BUILD_SDL
INCS 	      	= $(EXTRAS)                                                   \
                  $(INCLUDES)                                                 \
                  sdl
endif


ifdef BUILD_WINLIN
INCS 	       += sdl/sdl
endif


ifdef BUILD_LINUX
INCS           += 'libpng-config --prefix'/include/libpng
endif


ifdef BUILD_GP2X
INCS 	       += sdl/gp2x
endif


INCS 	       += .                                                           \
                  source                                                      \
                  source/adpcmlib                                             \
                  source/gamelib                                              \
                  source/ramlib                                               \
                  source/randlib                                              \
                  source/scriptlib                                            \
                  source/tracelib


ifdef BUILD_DINGOO
INCS           += '/opt/mipsel-linux-uclibc/usr/bin/libpng-config --prefix'/include/libpng
endif


ifndef BUILD_DC
INCS 	       += source/pcxlib
endif


ifdef GFX
INCS 	       += source/gfxlib
endif



#------------------------------------------------------------------------------
# Objects
#------------------------------------------------------------------------------

ADPCM 	        = source/adpcmlib/adpcm.o

ifdef GFX
GFX 	        = source/gfxlib/2xSaI.o                                       \
                  source/gfxlib/bilinear.o                                    \
                  source/gfxlib/dotmatrix.o                                   \
                  source/gfxlib/gfx.o                                         \
                  source/gfxlib/hq2x.o                                        \
                  source/gfxlib/motionblur.o                                  \
                  source/gfxlib/scale2x.o                                     \
                  source/gfxlib/scanline.o                                    \
                  source/gfxlib/simple2x.o                                    \
                  source/gfxlib/tv2x.o
endif
		  
ifdef MMX
GFX 	       += source/gfxlib/2xSaImmx.o                                    \
                  source/gfxlib/bilinearmmx.o                                 \
                  source/gfxlib/hq2x16mmx.o
endif

GAME	        = source/gamelib/draw.o                                       \
                  source/gamelib/draw16.o                                     \
                  source/gamelib/draw32.o                                     \
                  source/gamelib/font.o                                       \
                  source/gamelib/anigif.o                                     \
                  source/gamelib/bitmap.o 	                              \
                  source/gamelib/screen.o                                     \
                  source/gamelib/screen16.o                                   \
                  source/gamelib/screen32.o                                   \
                  source/gamelib/loadimg.o                                    \
                  source/gamelib/palette.o                                    \
                  source/gamelib/packfile.o                                   \
                  source/gamelib/filecache.o                                  \
                  source/gamelib/pixelformat.o                                \
                  source/gamelib/soundmix.o                                   \
                  source/gamelib/spritef.o                                    \
                  source/gamelib/spriteq.o                                    \
                  source/gamelib/spritex8p16.o                                \
                  source/gamelib/spritex8p32.o                                \
                  source/gamelib/texture.o                                    \
                  source/gamelib/texture16.o                                  \
                  source/gamelib/texture32.o
SCRIPT          = source/scriptlib/StackedSymbolTable.o                       \
                  source/scriptlib/ScriptVariant.o                            \
                  source/scriptlib/SymbolTable.o                              \
                  source/scriptlib/Instruction.o                              \
                  source/scriptlib/Interpreter.o                              \
                  source/scriptlib/ParserSet.o                                \
                  source/scriptlib/Parser.o                                   \
                  source/scriptlib/Lexer.o                                    \
                  source/scriptlib/Stack.o                                    \
                  source/scriptlib/List.o
RAM             = source/ramlib/ram.o
RAND	        = source/randlib/rand32.o
TRACE           = source/tracelib/tracemalloc.o
SOURCE	        = source/utils.o                                              \
                  source/stristr.o


ifndef BUILD_DC
PCX             = source/pcxlib/savepcx.o
endif


ifdef BUILD_PSP
CONSOLE	        = psp/control/control.o                                       \
                  psp/dvemgr/dvemgr.o                                         \
                  psp/kernel/kernel.o                                         \
                  psp/graphics.o                                              \
                  psp/audiodrv.o                                              \
                  psp/sblaster.o                                              \
                  psp/control.o                                               \
                  psp/vertex.o                                                \
                  psp/timer.o                                                 \
                  psp/video.o                                                 \
                  psp/image.o                                                 \
                  psp/menu.o                                                  \
                  psp/pspport.o
endif


ifdef BUILD_DC
CONSOLE	        = dc/dcport.o                                                 \
                  dc/bios.o                                                   \
                  dc/gdrom.o                                                  \
                  dc/timer.o                                                  \
                  dc/sblaster.o                                               \
                  dc/control.o                                                \
                  dc/video.o
endif


ifdef BUILD_SDL
GAME	       += source/gamelib/filters.o
CONSOLE	        = sdl/joysticks.o                                             \
                  sdl/control.o                                               \
                  sdl/menu.o                                                  \
                  sdl/sblaster.o                                              \
                  sdl/sdlport.o                                               \
                  sdl/timer.o                                                 \
                  sdl/video.o
endif


ifdef BUILD_GP2X
CONSOLE	       += sdl/gp2x/gp2xport.o
endif




MAIN            = openborscript.o					      \
                  openbor.o

OBJS            = $(CONSOLE)                                                  \
                  $(ADPCM)                                                    \
                  $(GFX)                                                      \
                  $(GAME)                                                     \
                  $(PCX)                                                      \
                  $(SOURCE)                                                   \
                  $(SCRIPT)                                                   \
                  $(RAM)                                                      \
                  $(RAND)                                                     \
                  $(TRACE)                                                    \
                  $(MAIN)
		  
		  
#------------------------------------------------------------------------------
# Compiler Flags
#------------------------------------------------------------------------------

CFLAGS 	        = $(addprefix -I", $(addsuffix ", $(INCS)))


ifdef BUILD_DC
CFLAGS 	       += -O9 -DDC
else
CFLAGS 	       += -O2
endif


CFLAGS 	       += -g -Wall -Werror -fsigned-char -fomit-frame-pointer
CFLAGS 	       += -fno-ident -freorder-blocks


ifdef BUILD_PSP
CFLAGS 	       += -G0 -DPSP
CXXFLAGS        = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS         = $(CFLAGS)
endif	        


ifdef BUILD_WIN
CFLAGS 	       += -DMACHINE -DSDL -DWIN
endif


ifdef BUILD_LINUX
CFLAGS 	       += -DMACHINE -DSDL -DLINUX
endif


ifdef BUILD_GP2X
CFLAGS 	       += -DSDL -DGP2X
endif

ifdef BUILD_DINGOO
CFLAGS 	       +=  -D_REENTRANT -DSDL -DDINGOO 
endif


ifdef MMX
CFLAGS 	       += -DMMX
endif


ifdef OSX
CFLAGS 	       += -DOSX
endif


#------------------------------------------------------------------------------
# Library
#------------------------------------------------------------------------------

ifdef BUILD_PSP
LIBS 	        = -lpspgu -lpspaudio -lpsppower -lpsprtc
endif

#verificar
ifdef BUILD_SDL
LIBS            = -L:$(LIBRARIES) -Wl,-rpath,/usr/lib -lSDLmain -lSDL -lSDL_gfx -lSDL_image
endif


ifdef OSX
LIBS           += -framework Cocoa 
endif


ifdef BUILD_WINLIN
LIBS           += -mwindows
endif


ifdef BUILD_GP2LIN
LIBS           += -lpthread
endif


ifdef BUILD_GP2X
LIBS           += -static
endif

ifdef DINGOO
LIBS           += -static
endif



ifdef BUILD_DC
LIBS            = -lc -lgcc -lSDL_129
endif


LIBS           += -lpng -lz -lm


#------------------------------------------------------------------------------
# Rules to manage Files and Libraries for PSP
#------------------------------------------------------------------------------
       
ifdef BUILD_PSP
%.o : %.c
	@echo Compiling $(TARGET_PLATFORM) Port: $<...
	@$(CC) $(CFLAGS) -c $< -o $@
%.o : %.S
	@echo Compiling $(TARGET_PLATFORM) Port: $<...
	@$(CC) $(CFLAGS) -c $< -o $@
	
INCDIR          = $(INCS)
PSP_EBOOT_TITLE = $(VERSION_NAME) $(VERSION)
PSP_EBOOT_ICON 	= resources/OpenBOR_Icon_144x80.png
PSP_EBOOT_PIC1	= resources/OpenBOR_Logo_480x272.png
PSP_FW_VERSION  = 371
PSP_LARGE_MEMORY= 1
BUILD_PRX       = 1
include psp/build.mak
endif


#------------------------------------------------------------------------------
# Rules to manage Files and Libraries for Dreamcast
#------------------------------------------------------------------------------

ifdef BUILD_DC
all : $(TARGET) $(EXTRA_TARGETS)
KOS_LOCAL_CFLAGS = -I$(KOS_BASE)/../kos-ports/include -ffast-math
include $(KOS_BASE)/Makefile.rules
%.o : %.c
	@echo Compiling $(TARGET_PLATFORM) Port: $<...
	@$(KOS_CC) $(KOS_CFLAGS) $(CFLAGS) -c $< -o $@
%.o : %.s
	@echo Compiling $(TARGET_PLATFORM) Port: $<...
	@$(KOS_AS) $(KOS_AFLAGS) $< -o $@
%.o : %.S
	@echo Compiling $(TARGET_PLATFORM) Port: $<...
	@$(KOS_AS) $(KOS_AFLAGS) $< -o $@
$(TARGET) : $(OBJS)
	@echo
	@echo Linking $(TARGET_PLATFORM) Port: $(TARGET)...
	@$(KOS_CC) $(KOS_CFLAGS) $(KOS_LOCAL_CFLAGS) $(KOS_LDFLAGS) -o $@ $(KOS_START) $^ $(LIBS) $(KOS_LIBS)
$(EXTRA_TARGETS) : $(TARGET)
	@echo Creating $(TARGET_PLATFORM) Port: $(EXTRA_TARGETS)...
	@$(KOS_OBJCOPY) -R .stack -O binary $(TARGET) $(EXTRA_TARGETS)
	@echo
	@echo Completed $(TARGET_PLATFORM) Port!
	@echo $(EXTRA_TARGETS) is now ready!
endif


#------------------------------------------------------------------------------
# Rules to manage Files and Libraries for All Other TARGET_PLATFORMs
#------------------------------------------------------------------------------

ifdef BUILD_SDL
all : $(TARGET) $(EXTRA_TARGETS)
%.o : %.asm
	@echo Compiling $(TARGET_PLATFORM) Port: $<...
	@$(YASM) -D $(TARGET_PLATFORM) -f $(OBJTYPE) -o $@ $<
%.o : %.c
	@echo Compiling $(TARGET_PLATFORM) Port: $<...
	@$(CC) $(CFLAGS) -c $< -o $@
$(TARGET) : $(OBJS) $(RES)
	@echo
	@echo Linking $(TARGET_PLATFORM) Port: $(TARGET)...
	@$(CC) -o $(TARGET) $(OBJS) $(RES) $(LIBS)
$(EXTRA_TARGETS) : $(TARGET)
	@echo Stripping $(TARGET_PLATFORM) Port: $(EXTRA_TARGETS)...
	@$(STRIP) $(TARGET) -o $(EXTRA_TARGETS)
	@echo
	@echo Completed $(TARGET_PLATFORM) Port!
	@echo $(TARGET) is now ready!
endif


#------------------------------------------------------------------------------
# Rules to CleanUp Files for All TARGET_PLATFORMs
#------------------------------------------------------------------------------

ifndef BUILDING
all:
	@echo
	@echo Build A TARGET_PLATFORM:
	@echo
	@echo make BUILD_DC=1
	@echo make BUILD_PSP=1
	@echo make BUILD_PS2=1
	@echo make BUILD_WIN=1
	@echo make BUILD_GP2X=1
	@echo make BUILD_LINUX=1
	@echo make BUILD_DINGOO=1
	@echo
	@echo Cleanup Intermediate Files:
	@echo 
	@echo make clean
	@echo
	@echo Remove All Files:
	@echo 
	@echo make clean-all
	@echo
endif


ifndef BUILD_PSP
clean-all: clean-releases clean

clean-releases:
	@rm -rf releases/* 
	
clean:
	@echo
	@echo "Removing All $(TARGET_PLATFORM) Files..."
	@rm -f $(TARGET) $(EXTRA_TARGETS) PARAM.SFO linkmap $(OBJS)
	@echo Done!
	@echo
endif

version:
	@echo "-------------------------------------------------------"
	@echo "OpenBOR $(VERSION) - http://www.LavaLit.com"
	@echo 
	@echo "Licensed under the BSD license."
	@echo "See LICENSE and README within OpenBOR root for details."
	@echo 
	@echo "Copyright (c) 2004 - 2009 OpenBOR Team"
	@echo "-------------------------------------------------------"
