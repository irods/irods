#!/bin/bash

SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
DETECTEDOS=`$SCRIPTPATH/find_os.sh`
if [ "$DETECTEDOS" == "Ubuntu" ] ; then
    OSVERSION=`grep RELEASE /etc/lsb-release | awk -F= '{print $2}'`
elif [ "$DETECTEDOS" == "Debian" ] ; then
    OSVERSION=`cat /etc/debian_version | awk -F= '{print $1}'`
elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
    OSVERSION=`awk '{print $3}' /etc/redhat-release`
elif [ "$DETECTEDOS" == "SuSE" ] ; then
    OSVERSION=`grep VERSION /etc/SuSE-release | awk '{print $3}'`
elif [ "$DETECTEDOS" == "Solaris" ] ; then
    OSVERSION=`uname -r`
elif [ "$DETECTEDOS" == "MacOSX" ] ; then
    OSVERSION=`sw_vers -productVersion`
elif [ "$DETECTEDOS" == "ArchLinux" ] ; then
    OSVERSION=`grep RELEASE /etc/lsb-release | awk -F= '{print $2}'`
else
    exit 1
fi

echo "$OSVERSION"
exit 0

