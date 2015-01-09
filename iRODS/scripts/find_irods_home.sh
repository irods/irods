#!/bin/bash
#################
# This script is assumed to be run by build.sh
#################

if [ "$1" = "runinplace" ] ; then
    SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
    cd $SCRIPTPATH/../
    echo `pwd`
else
    echo /var/lib/irods/iRODS
fi
