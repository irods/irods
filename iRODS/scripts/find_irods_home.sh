#!/bin/sh
#################
# This script is assumed to be run by build.sh
#################

SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
cd $SCRIPTPATH/../

if [ "$1" = "runinplace" ] ; then
    echo `pwd`
else
    CFG=`find . -name "irods.config"`
    grep "IRODS_HOME =" $CFG | awk -F\' '{print $2}'
fi
