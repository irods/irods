#!/bin/bash -e

# get into the top level directory
DETECTEDDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DETECTEDDIR/../

# detect run-in-place installation
if [ -e /etc/irods/server_config.json ] ;  then
    RUNINPLACE=0
    # set configuration file paths
    IRODS_CONFIG_FILE="/etc/irods/irods.config"
    SERVER_CONFIG_FILE="/etc/irods/server_config.json"
elif [ -e /etc/irods/server.config ] ;  then
    RUNINPLACE=0
    # set configuration file paths
    IRODS_CONFIG_FILE="/etc/irods/irods.config"
    SERVER_CONFIG_FILE="/etc/irods/server.config"
else
    RUNINPLACE=1
    # set configuration file paths
    IRODS_CONFIG_FILE=$DETECTEDDIR/../iRODS/config/irods.config
    SERVER_CONFIG_FILE=$DETECTEDDIR/../iRODS/server/config/server_config.json
    # clean up full paths
    IRODS_CONFIG_FILE="$(cd "$( dirname $IRODS_CONFIG_FILE )" && pwd)"/"$( basename $IRODS_CONFIG_FILE )"
    SERVER_CONFIG_FILE="$(cd "$( dirname $SERVER_CONFIG_FILE )" && pwd)"/"$( basename $SERVER_CONFIG_FILE )"
    # get configuration information
    set +e
    source ./packaging/setup_irods_configuration.sh 2> /dev/null
    set -e
fi

# get temp file from prior run, if it exists
SETUP_RESOURCE_FLAG="/tmp/$USER/setup_resource.flag"
if [ -f $SETUP_RESOURCE_FLAG ] ; then
    # have run this before, read the existing config file
    ICATHOST=`grep "IRODS_ICAT_HOST =" $IRODS_CONFIG_FILE | awk -F\' '{print $2}'`
    ICATZONE=`grep "ZONE_NAME =" $IRODS_CONFIG_FILE | awk -F\' '{print $2}'`
    STATUS="loop"
else
    # no temp file, this is the first run
    STATUS="firstpass"
fi

echo "==================================================================="
echo ""
echo "You are installing an iRODS resource server.  Resource servers"
echo "cannot be started until they have been properly configured to"
echo "communicate with a live iCAT server."
echo ""
while [ "$STATUS" != "complete" ] ; do

  # set default values from an earlier loop
  if [ "$STATUS" != "firstpass" ] ; then
    LASTICATHOST=$ICATHOST
    LASTICATZONE=$ICATZONE
  fi

  # get host
  echo -n "iCAT server's hostname"
  if [ "$LASTICATHOST" ] ; then echo -n " [$LASTICATHOST]"; fi
  echo -n ": "
  read ICATHOST
  if [ "$ICATHOST" == "" ] ; then
    if [ "$LASTICATHOST" ] ; then ICATHOST=$LASTICATHOST; fi
  fi
  # strip all forward slashes
  ICATHOST=`echo "${ICATHOST}" | sed -e "s/\///g"`
  echo ""

  # get zone
  echo -n "iCAT server's ZoneName"
  if [ "$LASTICATZONE" ] ; then echo -n " [$LASTICATZONE]"; fi
  echo -n ": "
  read ICATZONE
  if [ "$ICATZONE" == "" ] ; then
    if [ "$LASTICATZONE" ] ; then ICATZONE=$LASTICATZONE; fi
  fi
  # strip all forward slashes
  ICATZONE=`echo "${ICATZONE}" | sed -e "s/\///g"`
  echo ""

  # confirm
  echo "-------------------------------------------"
  echo "iCAT Host:    $ICATHOST"
  echo "iCAT Zone:    $ICATZONE"
  echo "-------------------------------------------"
  echo -n "Please confirm these settings [yes]: "
  read CONFIRM
  if [ "$CONFIRM" == "" -o "$CONFIRM" == "y" -o "$CONFIRM" == "Y" -o "$CONFIRM" == "yes" ]; then
    STATUS="complete"
  else
    STATUS="loop"
  fi
  echo ""
  echo ""

done
touch $SETUP_RESOURCE_FLAG
echo "==================================================================="

FIRSTHALF=`hostname -s`
SECONDHALF="Resource"
LOCAL_RESOURCE_NAME="$FIRSTHALF$SECONDHALF"
THIRDHALF="Vault"
LOCAL_VAULT_NAME="$LOCAL_RESOURCE_NAME$THIRDHALF"
IRODS_CONFIG_TEMPFILE="/tmp/tmp.irods.config"

echo "Updating irods.config..."
sed -e "/^\$IRODS_ICAT_HOST/s/^.*$/\$IRODS_ICAT_HOST = '$ICATHOST';/" $IRODS_CONFIG_FILE > $IRODS_CONFIG_TEMPFILE
mv $IRODS_CONFIG_TEMPFILE $IRODS_CONFIG_FILE
sed -e "/^\$ZONE_NAME/s/^.*$/\$ZONE_NAME = '$ICATZONE';/" $IRODS_CONFIG_FILE > $IRODS_CONFIG_TEMPFILE
mv $IRODS_CONFIG_TEMPFILE $IRODS_CONFIG_FILE

# set a default initial resource name
sed -e "/^\$RESOURCE_NAME/s/^.*$/\$RESOURCE_NAME = '$LOCAL_RESOURCE_NAME';/" $IRODS_CONFIG_FILE > $IRODS_CONFIG_TEMPFILE
mv $IRODS_CONFIG_TEMPFILE $IRODS_CONFIG_FILE

# # set a default initial resource directory name (vault path)
# if [ $RUNINPLACE -eq 1 ] ; then
#     # run-in-place vault
#     RUNINPLACE_RESOURCE_DIR=$DETECTEDDIR/../iRODS/$LOCAL_VAULT_NAME
#     echo "before - runinplace resource dir [$RUNINPLACE_RESOURCE_DIR]"
#     RUNINPLACE_RESOURCE_DIR="$(cd "$( dirname $RUNINPLACE_RESOURCE_DIR )" && pwd)"/"$( basename $RUNINPLACE_RESOURCE_DIR )"
#     echo "after - runinplace resource dir [$RUNINPLACE_RESOURCE_DIR]"
#     sed -e "/^\$RESOURCE_DIR/s/^.*$/\$RESOURCE_DIR = '$RUNINPLACE_RESOURCE_DIR';/" $IRODS_CONFIG_FILE > $IRODS_CONFIG_TEMPFILE
# else
#     # package installation vault
#     sed -e "/^\$RESOURCE_DIR/s/^.*$/\$RESOURCE_DIR = '\/var\/lib\/irods\/iRODS\/$LOCAL_VAULT_NAME';/" $IRODS_CONFIG_FILE > $IRODS_CONFIG_TEMPFILE
# fi
# mv $IRODS_CONFIG_TEMPFILE $IRODS_CONFIG_FILE

echo "Running irods_setup.pl..."
cd iRODS
if [ $# -eq 1 ] ; then
    # for devtest in the cloud
    perl ./scripts/perl/irods_setup.pl $1
else
    # manual
    perl ./scripts/perl/irods_setup.pl
fi

