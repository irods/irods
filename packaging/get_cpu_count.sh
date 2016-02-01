#!/bin/bash

SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
DETECTEDOS=`bash $SCRIPTPATH/find_os.sh`
if [ "$DETECTEDOS" == "MacOSX" ] ; then
    DETECTEDCPUCOUNT=`sysctl -n hw.ncpu`
elif [ "$DETECTEDOS" == "Solaris" ] ; then
    DETECTEDCPUCOUNT=`/usr/sbin/psrinfo -p`
else
    DETECTEDCPUCOUNT=`cat /proc/cpuinfo | grep processor | wc -l | tr -d ' '`
fi
if [ $DETECTEDCPUCOUNT -lt 2 ] ; then
    DETECTEDCPUCOUNT=1
fi
echo $DETECTEDCPUCOUNT

exit 0
