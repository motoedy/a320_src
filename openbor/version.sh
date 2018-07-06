#
# OpenBOR - http://www.LavaLit.com
# -----------------------------------------------------------------------
# Licensed under the BSD license, see LICENSE in OpenBOR root for details.
#
# Copyright (c) 2004 - 2009 OpenBOR Team
#

#!/bin/bash
# Script acquires the verison number from SVN Repository and creates 
# a version.h as well as the environment variable to be used.

function check_svn_bin {
if [ `tools/bin/echo $(uname -s) | tools/bin/grep -o "windows"` ]; then
  if [ ! -d "tools/svn/bin" ]; then
    echo "-------------------------------------------------------"
    echo "           SVN - Not Found, Installing SVN!"
    echo "-------------------------------------------------------"
    tools/7-Zip/7za x -y tools/svn/svn-win32-1.6.6.7z -otools/svn/
    echo
    echo "-------------------------------------------------------"
    echo "           SVN - Installation Has Completed!"
    echo "-------------------------------------------------------"
  fi
fi
}

function read_version {
check_svn_bin
VERSION_NAME="OpenBOR"
VERSION_MAJOR=3
VERSION_MINOR=0
VERSION_BUILD=`svn info | grep "Last Changed Rev" | sed s/Last\ Changed\ Rev:\ //g`
export VERSION="v$VERSION_MAJOR.$VERSION_MINOR Build $VERSION_BUILD"
}

function write_version {
rm -rf version.h
echo "/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#ifndef VERSION_H
#define VERSION_H

#define VERSION_NAME \"$VERSION_NAME\"
#define VERSION_MAJOR \"$VERSION_MAJOR\"
#define VERSION_MINOR \"$VERSION_MINOR\"
#define VERSION_BUILD \"$VERSION_BUILD\"
#define VERSION (\"v\"VERSION_MAJOR\".\"VERSION_MINOR\" Build \"VERSION_BUILD)

#endif" >> version.h
}

function archive_release {
svn log --verbose > ./releases/VERSION_INFO.txt
7za a -t7z -mx9 -r -x!.svn "./releases/OpenBOR $VERSION.7z" ./releases/*
}

case $1 in
1)
    read_version
    echo ------------------------------------------------------
    echo "      Creating Archive OpenBOR $VERSION.7z"
    echo ------------------------------------------------------
    archive_release
    ;;
*)
    read_version
    write_version
    ;;
esac