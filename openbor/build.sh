#
# OpenBOR - http://www.LavaLit.com
# -----------------------------------------------------------------------
# Licensed under the BSD license, see LICENSE in OpenBOR root for details.
#
# Copyright (c) 2004 - 2009 OpenBOR Team
#

#!/bin/bash
# Building script for all platforms
# build.sh by SX (SumolX@gmail.com)

# Display Version
function version {
  . version.sh
  make version
  cp README ./releases/README.txt
  cp LICENSE ./releases/LICENSE.txt
  cp COMPILING ./releases/COMPILING.txt
}

# CleanUp Releases
function clean {
  make clean-releases
}

# Distribute Releases
function distribute {
  ERROR=0
  echo ------------------------------------------------------
  echo "          Validating Platforms Built w/Bash"
  echo ------------------------------------------------------
  echo
  
  if ! test "releases/PSP/OpenBOR/EBOOT.PBP"; then
    echo "PSP Platform Failed To Build!"
    ERROR=1
  fi
  if ! test -e "releases/GP2X/OpenBOR/OpenBOR.gpe"; then
    echo "GP2X Platform Failed To Build!"
    ERROR=1
  fi
  if ! test -e "releases/DC/OpenBOR/1ST_READ.BIN"; then
    echo "Dreamcast Platform Failed To Build!"
    ERROR=1
  fi
  if ! test -e "releases/WINDOWS_DLL/OpenBOR/OpenBOR.exe"; then
    echo "Windows DLL Platform Failed To Build!"
    ERROR=1
  fi
  
  if [ $ERROR = 0 ]; then
    echo "All Platforms Created Successfully"
    if [ $BUILDBATCH = 0 ]; then
      svn log --verbose > ./releases/VERSION_INFO.txt
      7za a -t7z -mx9 -r -x!.svn "./releases/OpenBOR $VERSION.7z" ./releases/*
    fi
  fi
  echo
}

# PSP Environment && Compile
function psp {
  . environ.sh 1
  if test $PSPDEV; then
    make clean BUILD_PSP=1
    make BUILD_PSP=1
    mkdir ./releases/PSP
    mkdir ./releases/PSP/OpenBOR
    mkdir ./releases/PSP/OpenBOR/Images
    mkdir ./releases/PSP/OpenBOR/Logs
    mkdir ./releases/PSP/OpenBOR/Paks
    mkdir ./releases/PSP/OpenBOR/Saves
    mkdir ./releases/PSP/OpenBOR/Modules
    mv EBOOT.PBP ./releases/PSP/OpenBOR/
    mv OpenBOR.elf ./releases/PSP/OpenBOR/Modules/
    cp ./psp/dvemgr/dvemgr.prx ./releases/PSP/OpenBOR/Modules/
    cp ./psp/kernel/kernel.prx ./releases/PSP/OpenBOR/Modules/
    cp ./psp/control/control.prx ./releases/PSP/OpenBOR/Modules/
    cp ./psp/exception/exception.prx ./releases/PSP/OpenBOR/Modules/
    cp ./resources/OpenBOR_Menu_480x272_Sony.png ./releases/PSP/OpenBOR/Images/Menu.png
    cp ./resources/OpenBOR_Logo_480x272.png ./releases/PSP/OpenBOR/Images/Loading.png
    make clean BUILD_PSP=1
  fi
}

# PS2 Environment && Compile
function ps2 {
  . environ.sh 2
  if test $PS2DEV; then
    make clean BUILD_PS2=1
    make BUILD_PS2=1
    mkdir ./releases/PS2
    mkdir ./releases/PS2/OpenBOR
    mkdir ./releases/PS2/OpenBOR/Images
    mkdir ./releases/PS2/OpenBOR/Logs
    mkdir ./releases/PS2/OpenBOR/Paks
    mkdir ./releases/PS2/OpenBOR/Saves
    mkdir ./releases/PS2/OpenBOR/ScreenShots
    mv EBOOT.PBP ./releases/PS2/OpenBOR/
    cp ./ps2/data/Menu.png ./releases/PS2/OpenBOR/Images/
    make clean BUILD_PS2=1
  fi 
}

# Gp2x Environment && Compile
function gp2x {
  . environ.sh 3
  if test $GP2XDEV; then
    make clean BUILD_GP2X=1
    make BUILD_GP2X=1
    mkdir ./releases/GP2X
    mkdir ./releases/GP2X/OpenBOR
    mkdir ./releases/GP2X/OpenBOR/Logs
    mkdir ./releases/GP2X/OpenBOR/Paks  
    mkdir ./releases/GP2X/OpenBOR/Saves
    mkdir ./releases/GP2X/OpenBOR/ScreenShots
    cp ./sdl/gp2x/modules/mmuhack.o ./releases/GP2X/OpenBOR/
    mv OpenBOR.gpe ./releases/GP2X/OpenBOR/
    make clean BUILD_GP2X=1
  fi
}

# Linux Environment && Compile
function linux {
  . environ.sh 4
  if test $LNXDEV; then
    make clean BUILD_LINUX=1
    make BUILD_LINUX=1
    if [ `echo $PLATFORM | grep -o "Darwin"` ]; then
      mkdir ./releases/MAC
      mkdir ./releases/MAC/OpenBOR.app
      mkdir ./releases/MAC/OpenBOR.app/Contents
      mkdir ./releases/MAC/OpenBOR.app/Contents/MacOS
      mkdir ./releases/MAC/OpenBOR.app/Contents/Resources
      mv OpenBOR ./releases/MAC/OpenBOR.app/Contents/MacOS
      cp ./resources/Pkginfo ./releases/MAC/OpenBOR.app/Contents
      cp ./resources/Info.plist ./releases/MAC/OpenBOR.app/Contents
      cp ./resources/OpenBOR.icns ./releases/MAC/OpenBOR.app/Contents/Resources
    else
      mkdir ./releases/LINUX
      mkdir ./releases/LINUX/OpenBOR
      mkdir ./releases/LINUX/OpenBOR/Logs
      mkdir ./releases/LINUX/OpenBOR/Paks
      mkdir ./releases/LINUX/OpenBOR/Saves
      mkdir ./releases/LINUX/OpenBOR/ScreenShots
      mv OpenBOR ./releases/LINUX/OpenBOR  
    fi
    make clean BUILD_LINUX=1
  fi
}

# Windows Environment && Compile
function windows {
  . environ.sh 5
  if test $WINDEV; then
    make clean BUILD_WIN=1
    make BUILD_WIN=1
    mkdir ./releases/WINDOWS_DLL
    mkdir ./releases/WINDOWS_DLL/OpenBOR
    mkdir ./releases/WINDOWS_DLL/OpenBOR/Logs
    mkdir ./releases/WINDOWS_DLL/OpenBOR/Paks
    mkdir ./releases/WINDOWS_DLL/OpenBOR/Saves
    mkdir ./releases/WINDOWS_DLL/OpenBOR/ScreenShots
    cp ./sdl/zlib1.dll ./releases/WINDOWS_DLL/OpenBOR/
    cp ./sdl/libpng12.dll ./releases/WINDOWS_DLL/OpenBOR/
    cp ./sdl/libpng13.dll ./releases/WINDOWS_DLL/OpenBOR/
    cp ./sdl/jpeg62.dll ./releases/WINDOWS_DLL/OpenBOR/
    cp ./sdl/tiff.dll ./releases/WINDOWS_DLL/OpenBOR/
    cp ./sdl/SDL.dll ./releases/WINDOWS_DLL/OpenBOR/
    cp ./sdl/SDL_gfx.dll ./releases/WINDOWS_DLL/OpenBOR/
    cp ./sdl/SDL_Image.dll ./releases/WINDOWS_DLL/OpenBOR/
    mv OpenBOR.exe ./releases/WINDOWS_DLL/OpenBOR
    make clean BUILD_WIN=1
  fi
}

# Dreamcast Environment && Compile
function dreamcast {
  . environ.sh 6
  if test $KOS_BASE; then
    make clean BUILD_DC=1
    make BUILD_DC=1
    mkdir ./releases/DC
    mkdir ./releases/DC/OpenBOR
    mv 1ST_READ.BIN ./releases/DC/OpenBOR/
    make clean BUILD_DC=1
  fi
}

case $1 in
1)
    version
    psp
    ;;
2)
    version
    ps2
    ;;
3)
    version
    gp2x
    ;;
4)
    version
    linux
    ;;
5)
    version
    windows
    ;;
6)
    version
    dreamcast
    ;;
7)
    version
    distribute
    ;;
?)
   version
   echo
   echo "-------------------------------------------------------"
   echo "    1 = PSP"
   echo "    2 = PS2"
   echo "    3 = Gp2x"
   echo "    4 = Linux"
   echo "    5 = Windows"
   echo "    6 = Dreamcast"
   echo "    7 = Distribute"
   echo "-------------------------------------------------------"
   echo
   ;;
*)
    clean
    version
    if test -e "buildspec.sh"; then
        . buildspec.sh
    else
        psp
        #ps2
        gp2x
        linux
        windows
        dreamcast
    fi
    distribute
    ;;
esac
