#!/bin/bash

# detect operating system
UNAMERESULTS=`uname`
if [ -f "/etc/redhat-release" ]; then       # CentOS and RHEL and Fedora
    DETECTEDOS="RedHatCompatible"
elif [ -f "/etc/SuSE-release" ]; then       # SuSE
    DETECTEDOS="SuSE"
elif [ -f "/etc/lsb-release" ]; then        # Ubuntu
    DETECTEDOS="Ubuntu"
elif [ -f "/usr/bin/sw_vers" ]; then        # MacOSX
    DETECTEDOS="MacOSX"
elif [ "$UNAMERESULTS" == "SunOS" ]; then   # Solaris
    DETECTEDOS="Solaris"
fi

echo "$DETECTEDOS"

exit 0
