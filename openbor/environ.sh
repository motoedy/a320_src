#
# OpenBOR - http://www.LavaLit.com
# -----------------------------------------------------------------------
# Licensed under the BSD license, see LICENSE in OpenBOR root for details.
#
# Copyright (c) 2004 - 2009 OpenBOR Team
#

#!/bin/bash
# Environments for Specific HOST_PLATFORMs
# environ.sh by SX (SumolX@gmail.com)

export HOST_PLATFORM=$(uname -s)
export MACHINENAME=$(uname -m)
export TOOLS=tools/bin:tools/7-Zip:tools/svn/bin

if [ `echo $MACHINENAME | grep -o "ppc64"` ]; then
  export MACHINE=__ppc__
elif [ `echo $MACHINENAME | grep -o "powerpc"` ]; then
  export MACHINE=__powerpc__
elif [ `echo $MACHINENAME | grep -o "M680*[0-9]0"` ]; then
  export MACHINE=__${MACHINENAME}__
elif [ `echo $MACHINENAME | grep -o "i*[0-9]86"` ]; then
  export MACHINE=__${MACHINENAME%%-*}__
elif [ `echo $MACHINENAME | grep -o "x86"` ]; then
  export MACHINE=__${MACHINENAME%%-*}__
fi

case $1 in

############################################################################
#                                                                          #
#                           PSP Environment                                #
#                                                                          #
############################################################################
1) 
   if test -e "C:/pspsdk"; then
     export PSPDEV=C:/pspsdk
     export PATH=$PATH:$PSPDEV/bin
   elif test -e "c:/Cygwin/usr/local/pspdev"; then
     export PSPDEV=c:/Cygwin/usr/local/pspdev
     export PATH=$PATH:$PSPDEV/bin:C:/Cygwin/bin
   elif test -e "/usr/local/pspdev"; then
     export PSPDEV=/usr/local/pspdev
     export PATH=$PATH:$PSPDEV/bin
   elif [ `echo $HOST_PLATFORM | grep -o "windows"` ]; then
     if [ ! -d "tools/psp-sdk/bin" ]; then
       echo "-------------------------------------------------------"
       echo "        PSP SDK - Not Found, Installing SDK!"
       echo "-------------------------------------------------------"
       tools/7-Zip/7za.exe x -y tools/psp-sdk/psp-sdk.7z -otools/psp-sdk/
       echo
       echo "-------------------------------------------------------"
       echo "        PSP SDK - Installation Has Completed!"
       echo "-------------------------------------------------------"
     fi
       export PSPDEV=tools/psp-sdk
       export PATH=$TOOLS:$PSPDEV/bin
       HOST_PLATFORM="SVN";
   fi
   if test $PSPDEV; then
     export PSPSDK=$PSPDEV/psp/sdk
     echo "-------------------------------------------------------"
     echo "          PSP SDK ($HOST_PLATFORM) Environment Loaded!"
     echo "-------------------------------------------------------"
   else
     echo "-------------------------------------------------------"
     echo "            ERROR - PSP Environment Failed"
     echo "                   SDK Installed?"
     echo "-------------------------------------------------------"
   fi
   ;;

############################################################################
#                                                                          #
#                              PS2 Environment                             #
#                                                                          #
############################################################################
2)
   if test -e "c:/Cygwin/usr/local/ps2dev"; then
     export PS2DEV=c:/Cygwin/usr/local/ps2dev
     export PS2SDK=$PS2DEV/ps2sdk
   elif test -e "/usr/local/ps2dev"; then
     export PS2DEV=/usr/local/ps2dev
     export PS2SDK=$PS2DEV/ps2sdk
   elif [ `echo $HOST_PLATFORM | grep -o "windows"` ]; then
     if [ ! -d "tools/ps2-sdk/bin" ]; then
       echo "-------------------------------------------------------"
       echo "        PS2 SDK - Not Found, Installing SDK!"
       echo "-------------------------------------------------------"
       tools/7-Zip/7za.exe x -y tools/ps2-sdk/ps2-sdk.7z -otools/ps2-sdk/
       echo
       echo "-------------------------------------------------------"
       echo "        PS2 SDK - Installation Has Completed!"
       echo "-------------------------------------------------------"
     fi
     export PS2DEV=tools/ps2-sdk
     export PS2SDK=$PS2DEV
     HOST_PLATFORM="SVN"
   fi
   if test $PS2DEV; then
     export PATH=$PATH:$PS2DEV/bin
     export PATH=$PATH:$PS2DEV/ee/bin
     export PATH=$PATH:$PS2DEV/iop/bin
     export PATH=$PATH:$PS2DEV/dvp/bin
     if [ $HOST_PLATFORM = SVN ]; then 
       export PATH=$TOOLS:$PS2SDK/bin
     else
       export PATH=$PATH:$PS2SDK/bin
     fi
     echo "-------------------------------------------------------"
     echo "          PS2 SDK ($HOST_PLATFORM) Environment Loaded!"
     echo "-------------------------------------------------------"
   else
     echo "-------------------------------------------------------"
     echo "            ERROR - PS2 Environment Failed"
     echo "                   SDK Installed?"
     echo "-------------------------------------------------------"
   fi
   ;;

############################################################################
#                                                                          #
#                            GP2X Environment                              #
#                                                                          #
############################################################################
3)
   if test -e "c:/Cygwin/opt/open2x/gcc-4.1.1-glibc-2.3.6/bin/arm-open2x-linux-gcc.exe"; then
     export GP2XDEV=c:/Cygwin/opt/open2x/gcc-4.1.1-glibc-2.3.6/bin
     export SDKPATH=c:/Cygwin/opt/open2x/gcc-4.1.1-glibc-2.3.6
     export PATH=$PATH:$GP2XDEV
   elif test -e "/opt/open2x/gcc-4.1.1-glibc-2.3.6/bin/arm-open2x-linux-gcc"; then
     export GP2XDEV=/opt/open2x/gcc-4.1.1-glibc-2.3.6/bin
     export SDKPATH=/opt/open2x/gcc-4.1.1-glibc-2.3.6
     export PATH=$PATH:$GP2XDEV
   elif test -e "c:/Cygwin/opt/open2x/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/bin/arm-open2x-linux-gcc.exe"; then
     export GP2XDEV=/opt/open2x/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/bin
     export SDKPATH=/opt/open2x/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux
     export PATH=$PATH:$GP2XDEV
   elif test -e "/opt/open2x/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/bin/arm-open2x-linux-gcc"; then
     export GP2XDEV=/opt/open2x/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/bin
     export SDKPATH=/opt/open2x/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux
     export PATH=$PATH:$GP2XDEV
   elif [ `echo $HOST_PLATFORM | grep -o "windows"` ]; then
     if [ ! -d "tools/gp2x-sdk/bin" ]; then
       echo "-------------------------------------------------------"
       echo "         GP2X SDK - Not Found, Installing SDK!"
       echo "-------------------------------------------------------"
       tools/7-Zip/7za.exe x -y tools/gp2x-sdk/gp2x-sdk.7z -otools/gp2x-sdk/
       echo
       echo "-------------------------------------------------------"
       echo "         GP2X SDK - Installation Has Completed!"
       echo "-------------------------------------------------------"
     fi
     export GP2XDEV=tools/gp2x-sdk/bin
     export SDKPATH=tools/gp2x-sdk/arm-open2x-linux
     export PATH=$TOOLS:$GP2XDEV
     HOST_PLATFORM="SVN"
   fi
   if test $GP2XDEV; then
     echo "-------------------------------------------------------"
     echo "           GP2X SDK ($HOST_PLATFORM) Environment Loaded!"
     echo "-------------------------------------------------------"
   else
     echo "-------------------------------------------------------"
     echo "            ERROR - GP2X Environment Failed"
     echo "                   SDK Installed?"
     echo "-------------------------------------------------------"
   fi
   ;;

############################################################################
#                                                                          #
#                           Linux Environment                              #
#                                                                          #
############################################################################
4)
   if [ `echo $HOST_PLATFORM | grep -o "Linux"` ]; then
     if test -e "/sw/bin"; then
       export LNXDEV=/sw
     elif test -e "/usr/bin"; then
       export LNXDEV=/usr
     fi
     if test $LNXDEV; then
       echo "-------------------------------------------------------"
       echo "         $HOST_PLATFORM $MACHINENAME Environment Loaded!"
       echo "-------------------------------------------------------"
     else
       echo "-------------------------------------------------------"
       echo "          ERROR - $HOST_PLATFORM Environment Failed"
       echo "                   SDK Installed?"
       echo "-------------------------------------------------------"
     fi
   fi
   ;;

############################################################################
#                                                                          #
#                           Windows Environment                            #
#                                                                          #
############################################################################
5)
   if test -e "c:/dev-cpp/bin"; then
     export WINDEV=c:/dev-cpp/bin
     export SDKPATH=c:/dev-cpp
   elif [ `echo $HOST_PLATFORM | grep -o "windows"` ]; then
     if [ ! -d "tools/win-sdk/bin" ]; then
       echo "-------------------------------------------------------"
       echo "      Windows SDK - Not Found, Installing SDK!"
       echo "-------------------------------------------------------"
       tools/7-Zip/7za.exe x -y tools/win-sdk/win-sdk.7z -otools/win-sdk/
       echo
       echo "-------------------------------------------------------"
       echo "      Windows SDK - Installation Has Completed!"
       echo "-------------------------------------------------------"
     fi
     export WINDEV=tools/win-sdk/bin
     export SDKPATH=tools/win-sdk/
     export PATH=$TOOLS:$WINDEV
     HOST_PLATFORM="SVN";
   fi
   if test $WINDEV; then
       echo "-------------------------------------------------------"
       echo "     Windows SDK ($HOST_PLATFORM) $MACHINENAME Environment Loaded!"
       echo "-------------------------------------------------------"
   else
       echo "-------------------------------------------------------"
       echo "          ERROR - Windows Environment Failed"
       echo "                   SDK Installed?"
       echo "-------------------------------------------------------"
   fi
   ;;

############################################################################
#                                                                          #
#                           Dreamcast Environment                          #
#                                                                          #
############################################################################
6)
   if test -e "/usr/local/dcdev/kos"; then
     . /usr/local/dcdev/kos/environ.sh
   elif [ `echo $HOST_PLATFORM | grep -o "windows"` ]; then
     if [ ! -d "tools/dc-sdk/kos" ]; then
        echo "-------------------------------------------------------"
        echo "     Dreamcast SDK - Not Found, Installing SDK!"
        echo "-------------------------------------------------------"
        tools/7-Zip/7za.exe x -y tools/dc-sdk/kos-svn-646.7z -otools/dc-sdk/
        echo
        echo "-------------------------------------------------------"
        echo "     Dreamcast SDK - Installation Has Completed!"
        echo "-------------------------------------------------------"
     fi
     HOST_PLATFORM="SVN";
     . tools/dc-sdk/kos/environ.sh
     export PATH=$TOOLS:$PATH     
   fi
   if test $KOS_BASE; then
     echo "-------------------------------------------------------"
     echo "          Dreamcast SDK ($HOST_PLATFORM) Environment Loaded!"
     echo "-------------------------------------------------------"
   else
     echo "-------------------------------------------------------"
     echo "         ERROR - Dreamcast Environment Failed"
     echo "                   SDK Installed?"
     echo "-------------------------------------------------------"
   fi
   ;;

############################################################################
#                                                                          #
#                             Wrong value?                                 #
#                                                                          #
############################################################################
*)
   echo
   echo "-------------------------------------------------------"
   echo "    Value Not Supported!"
   echo "    1 = PSP"
   echo "    2 = PS2"
   echo "    3 = Gp2x"
   echo "    4 = Linux"
   echo "    5 = Windows"
   echo "    6 = Dreamcast"
   echo "-------------------------------------------------------"
   echo
   ;;

esac
