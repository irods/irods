#!/bin/bash

# preflight
CPPCHECK=`which cppcheck`
if [[ "$?" != "0" || `echo $CPPCHECK | awk '{print $1}'` == "no" ]] ; then
    echo "cppcheck is required on this machine"
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then
        echo "try:"
        echo "  sudo apt-get install cppcheck"
    fi
    if [ "$DETECTEDOS" == "MacOSX" ] ; then
        echo "try:"
        echo "  brew install cppcheck"
    fi
    echo ""
    exit 1
fi

# find proper directory
DETECTEDDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DETECTEDDIR
H=`pwd`
echo "Detected Directory [$H]"

# detect operating system
DETECTEDOS=`$DETECTEDDIR/../packaging/find_os.sh`
echo "Detected OS [$DETECTEDOS]"


# find number of cpus
if [ "$DETECTEDOS" == "MacOSX" ] ; then
    DETECTEDCPUCOUNT=`sysctl -n hw.ncpu`
elif [ "$DETECTEDOS" == "Solaris" ] ; then
    DETECTEDCPUCOUNT=`/usr/sbin/psrinfo -p`
else
    DETECTEDCPUCOUNT=`cat /proc/cpuinfo | grep processor | wc -l`
fi
if [ "$DETECTEDCPUCOUNT" \< "2" ] ; then
    DETECTEDCPUCOUNT=1
fi
CPUCOUNT=$(( $DETECTEDCPUCOUNT + 3 ))
MAKEJCMD="make -j $CPUCOUNT"
echo "-----------------------------------"
echo "Detected CPUs:   $DETECTEDCPUCOUNT"
echo "Running with:    cppcheck -j $CPUCOUNT"
echo "-----------------------------------"
sleep 1

# run cppcheck
cppcheck --language=c++ -j $CPUCOUNT -f -I/usr/include -I$H/lib/hasher/include -I$H/lib/rbudp/include -I$H/lib/api/include -I$H/lib/core/include -I$H/lib/isio/include -I$H/server/re/include -I$H/server/drivers/include -I$H/server/core/include -I$H/server/icat/include -i clients/icommands/rulegen/ -i external .
