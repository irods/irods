#!/bin/bash

DETECTEDOS=`./find_os.sh`
if [ "$DETECTEDOS" == "Ubuntu" ] ; then
    OSVERSION=`grep RELEASE /etc/lsb-release | awk -F= '{print $2}'`
elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
    OSVERSION=`awk '{print $3}' /etc/redhat-release`
elif [ "$DETECTEDOS" == "SuSE" ] ; then
    OSVERSION=`grep VERSION /etc/SuSE-release | awk '{print $3}'`
elif [ "$DETECTEDOS" == "Solaris" ] ; then
    OSVERSION=`uname -r`
elif [ "$DETECTEDOS" == "MacOSX" ] ; then
    OSVERSION=`sw_vers -productVersion`
else
    exit 1
fi

echo "$OSVERSION"
exit 0

