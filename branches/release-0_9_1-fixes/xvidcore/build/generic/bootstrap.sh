#!/bin/sh
#
# This file builds the configure script and copies all needed files
# provided by automake/libtoolize
#
# $Id: bootstrap.sh,v 1.1.2.1 2003-03-17 23:24:20 edgomez Exp $


##############################################################################
# Detect the right autoconf script
##############################################################################

# Find a suitable autoconf
AUTOCONF="autoconf2.50"
$AUTOCONF --version 2>/dev/null 1>/dev/null

if [ $? -ne 0 ] ; then
    AUTOCONF="autoconf"
	$AUTOCONF --version 2>/dev/null 1>/dev/null

    if [ $? -ne 0 ] ; then
        echo "Autoconf not found"
        exit -1
    fi
fi

# Tests the autoconf version
AC_VER=`$AUTOCONF --version | head -1 | sed 's/'^[^0-9]*'/''/'`
AC_MAJORVER=`echo $AC_VER | cut -f1 -d'.'`
AC_MINORVER=`echo $AC_VER | cut -f2 -d'.'`

if [ "$AC_MAJORVER" -lt "2" ]; then
    echo "This bootstrapper needs Autoconf >= 2.50 (detected $AC_VER)"
    exit -1
fi

if [ "$AC_MINORVER" -lt "50" ]; then
    echo "This bootstrapper needs Autoconf >= 2.50 (detected $AC_VER)"
    exit -1
fi

##############################################################################
# Bootstraps the configure script
##############################################################################

echo "Creating ./configure"
$AUTOCONF

echo "Copying files provided by automake"
automake -c -a 1>/dev/null 2>/dev/null

echo "Copying files provided by libtool"
libtoolize -f -c 1>/dev/null 2>/dev/null

echo "Removing files that are not needed"
rm -rf autom4*
rm -rf ltmain.sh