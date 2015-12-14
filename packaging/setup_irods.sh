#!/bin/bash -e

# throw STDERR warning if short hostname found
MYHOST=`hostname`
if [[ ! $MYHOST == *.* ]] ; then
    echo "" 1>&2
    echo "********************************************************" 1>&2
    echo "*" 1>&2
    echo "* iRODS Setup WARNING:" 1>&2
    echo "*       hostname [$MYHOST] may need to be a FQDN" 1>&2
    echo "*" 1>&2
    echo "********************************************************" 1>&2
    echo "" 1>&2
fi

# locate current directory
DETECTEDDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

$DETECTEDDIR/../scripts/setup_irods.py
