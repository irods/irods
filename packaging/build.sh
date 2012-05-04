#!/bin/bash

SCRIPTNAME=`basename $0`

# check arguments
if [ $# -ne 1 -a $# -ne 2 ] ; then
  echo "Usage: $SCRIPTNAME icat {databasetype}   OR   $SCRIPTNAME resource"
  exit 1
fi

if [ $1 != "icat" -a $1 != "resource" ] ; then
  echo "Usage: $SCRIPTNAME icat {databasetype}   OR   $SCRIPTNAME resource"
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
    # need to do a dirname here, as the irods.config is expected to have a path
    # which will be appended with a /bin
    EIRODSPOSTGRESPATH=`../packaging/find_postgres_bin.sh`
    EIRODSPOSTGRESPATH=`dirname $EIRODSPOSTGRESPATH`
    EIRODSPOSTGRESPATH="$EIRODSPOSTGRESPATH/"

    echo "Detecting PostgreSQL Path: [$EIRODSPOSTGRESPATH]"
    sed -e s,EIRODSPOSTGRESPATH,$EIRODSPOSTGRESPATH, $EPMFILE > $TMPCONFIGFILE
  else
    echo "TODO: irods.config for DBTYPE other than postgres"
  fi

# set up variables for resource configuration
else

  SERVER_TYPE="RESOURCE"
  EPMFILE="../packaging/irods.config.resource.epm"
  cp $EPMFILE $TMPCONFIGFILE
fi



# =-=-=-=-=-=-=-
# run configure to create Makefile, config.mk, platform.mk, etc.
./scripts/configure
# overwrite with our values
cp $TMPCONFIGFILE ./config/irods.config
# run with our updated irods.config
./scripts/configure
# again to reset IRODS_HOME
cp $TMPCONFIGFILE ./config/irods.config
# handle issue with IRODS_HOME being overwritten by the configure script    
sed -e "\,^IRODS_HOME,s,^.*$,IRODS_HOME=\`./scripts/find_irods_home.sh\`," ./irodsctl > /tmp/irodsctl.tmp
mv /tmp/irodsctl.tmp ./irodsctl
chmod 755 ./irodsctl



# =-=-=-=-=-=-=-
# modify the eirods_ms_home.h file with the proper path to the binary directory
irods_msvc_home=`./scripts/find_irods_home.sh`
irods_msvc_home="$irods_msvc_home/server/bin/"
sed -e s,EIRODSMSCVPATH,$irods_msvc_home, server/re/include/eirods_ms_home.h.src > /tmp/eirods_ms_home.h
mv /tmp/eirods_ms_home.h server/re/include/




###########################################
# single 'make' time on a 8 core machine
###########################################
#        time make           1m55.508s
#        time make -j 1      1m55.023s
#        time make -j 2      0m17.199s
#        time make -j 3      0m11.873s
#        time make -j 4      0m9.894s   <-- inflection point
#        time make -j 5      0m9.164s
#        time make -j 6      0m8.515s
#        time make -j 7      0m8.042s
#        time make -j 8      0m7.898s
#        time make -j 9      0m7.911s
#        time make -j 10     0m7.898s
#        time make -j        0m30.920s
###########################################
make -j 4
make -j 4



# bake SQL files for different database types
if [ $1 == "icat" ] ; then
  if [ "$DB_TYPE" == "postgres" ] ; then
    echo "TODO: bake SQL for postgres"
  else
    echo "TODO: bake SQL for DBTYPE other than postgres"
  fi
fi



# generate randomized database password, replacing hardcoded placeholder
cd $DIR/../
RANDOMDBPASS=`cat /dev/urandom | base64 | head -c15`
sed -e "s,SOMEPASSWORD,$RANDOMDBPASS," ./packaging/e-irods.list.template > /tmp/eirodslist.tmp
mv /tmp/eirodslist.tmp ./packaging/e-irods.list



# run EPM for package type of this machine

# available from: http://fossies.org/unix/privat/epm-4.2-source.tar.gz
# md5sum 3805b1377f910699c4914ef96b273943

cd $DIR/../
if [ -f "/etc/redhat-release" ]; then # CentOS and RHEL and Fedora
  echo "Running EPM :: Generating RPM"
  epmvar="RPM$SERVER_TYPE" 
  epm -f rpm e-irods $epmvar=true $SERVER_TYPE=true RPM=true ./packaging/e-irods.list
elif [ -f "/etc/SuSE-release" ]; then # SuSE
  echo "Running EPM :: Generating RPM"
  epmvar="RPM$SERVER_TYPE" 
  epm -f rpm e-irods $epmvar=true $SERVER_TYPE=true RPM=true ./packaging/e-irods.list
elif [ -f "/etc/lsb-release" ]; then  # Ubuntu
  echo "Running EPM :: Generating DEB"
  epmvar="DEB$SERVER_TYPE" 
  epm -f deb e-irods $epmvar=true $SERVER_TYPE=true DEB=true ./packaging/e-irods.list
elif [ -f "/usr/bin/sw_vers" ]; then  # MacOSX
  echo "TODO: generate package for MacOSX"
fi






























