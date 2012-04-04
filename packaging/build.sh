#!/bin/bash

SCRIPTNAME=`basename $0`

# check arguments
if [ $# -ne 2 ] ; then
  echo "Usage: $SCRIPTNAME icat {databasetype}   OR   $SCRIPTNAME resource {icatip}"
  exit 1
fi

if [ $1 != "icat" -a $1 != "resource" ] ; then
  echo "Usage: $SCRIPTNAME icat {databasetype}   OR   $SCRIPTNAME resource {icatip}"
  exit 1
fi



# get into the correct directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/../iRODS



# set up own temporary configfile
TMPCONFIGFILE=/tmp/$USER/irods.config.epm
mkdir -p $(dirname $TMPCONFIGFILE)



# set up variables for icat configuration
if [ $1 == "icat" ] ; then

  SERVER_TYPE="ICAT"
  DB_TYPE=$2
  EPMFILE="../packaging/irods.config.icat.epm"
  if [ "$DB_TYPE" == "postgres" ] ; then
    EIRODSPOSTGRESPATH=`../packaging/find_postgres.sh | sed -e s,\/[^\/]*$,, -e s,\/[^\/]*$,,`
    EIRODSPOSTGRESPATH="$EIRODSPOSTGRESPATH/"
    echo "Detecting PostgreSQL Path: [$EIRODSPOSTGRESPATH]"
    sed -e s,EIRODSPOSTGRESPATH,$EIRODSPOSTGRESPATH, $EPMFILE > $TMPCONFIGFILE
  else
    echo "TODO: irods.config for DBTYPE other than postgres"
  fi

# set up variables for resource configuration
else

  SERVER_TYPE="RESOURCE"
  ICATIP=$2
  EPMFILE="../packaging/irods.config.resource.epm"
  sed -e s,REMOTEICATIPADDRESS,$ICATIP, $EPMFILE > $TMPCONFIGFILE

fi



# run configure to create Makefile, config.mk, platform.mk, etc.
./scripts/configure
# overwrite with our values
cp $TMPCONFIGFILE ./config/irods.config
# run with our updated irods.config
./scripts/configure
# again to reset IRODS_HOME
cp $TMPCONFIGFILE ./config/irods.config
# go!
make -j



# bake SQL files for different database types
if [ $1 == "icat" ] ; then
  if [ "$DB_TYPE" == "postgres" ] ; then
    echo "TODO: bake SQL for postgres"
  else
    echo "TODO: bake SQL for DBTYPE other than postgres"
  fi
fi



# run epm for all package types we're producing
cd $DIR/../
sudo epm -v -f deb e-irods $SERVER_TYPE=true DEB=true ./packaging/e-irods.list
sudo epm -v -f rpm e-irods $SERVER_TYPE=true RPM=true ./packaging/e-irods.list
