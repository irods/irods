#!/bin/bash

SCRIPTNAME=`basename $0`

# check arguments
if [ $# -ne 1 -a $# -ne 2 ] ; then
  echo "Usage: $SCRIPTNAME icat   OR   $SCRIPTNAME resource {icatip}"
  exit 1
fi

if [ $1 != "icat" -a $1 != "resource" ] ; then
  echo "Usage: $SCRIPTNAME icat   OR   $SCRIPTNAME resource {icatip}"
  exit 1
fi

if [ $1 == "icat" -a $# -eq 2 ] ; then
  echo "Usage: $SCRIPTNAME icat   OR   $SCRIPTNAME resource {icatip}"
  exit 1
fi

if [ $1 == "resource" -a $# -eq 1 ] ; then
  echo "Usage: $SCRIPTNAME icat   OR   $SCRIPTNAME resource {icatip}"
  exit 1
fi

# get into the correct directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/../iRODS


# set up variables for icat configuration
if [ $1 == "icat" ] ; then
  EPMFILE=../packaging/irods.config.icat.epm
  EIRODSPOSTGRESPATH=`../packaging/find_postgres.sh | sed -e s,\/[^\/]*$,, -e s,\/[^\/]*$,,`
  EIRODSPOSTGRESPATH="$EIRODSPOSTGRESPATH/"
  echo "Detecting PostgreSQL Path: [$EIRODSPOSTGRESPATH]"

  sed -e s,EIRODSPOSTGRESPATH,$EIRODSPOSTGRESPATH, $EPMFILE > /tmp/irods.config.epm


# set up variables for resource configuration
else

  EPMFILE=../packaging/irods.config.resource.epm
  ICATIP=$2

  sed -e s,REMOTEICATIPADDRESS,$ICATIP, $EPMFILE > /tmp/irods.config.epm

fi



# run configure to create Makefile, config.mk, platform.mk, etc.
./scripts/configure
# overwrite with our values
cp /tmp/irods.config.epm ./config/irods.config
# run with our updated irods.config
./scripts/configure
# again to reset IRODS_HOME
cp /tmp/irods.config.epm ./config/irods.config
# go!
make -j
