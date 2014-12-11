#!/bin/bash -e

# locate current directory
DETECTEDDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# service account config file
SERVICE_ACCOUNT_CONFIG_FILE="/etc/irods/service_account.config"

# define service account for this installation
if [ ! -f $SERVICE_ACCOUNT_CONFIG_FILE ] ; then
  $DETECTEDDIR/setup_irods_service_account.sh
fi

# import service account name and group
source $SERVICE_ACCOUNT_CONFIG_FILE

# configure irods
sudo su - $IRODS_SERVICE_ACCOUNT_NAME -c "$DETECTEDDIR/setup_irods_configuration.sh"

# if default vault path does not exist, create it with proper permissions
MYIRODSCONFIG=/etc/irods/irods.config
MYRESOURCEDIR=`grep "RESOURCE_DIR =" $MYIRODSCONFIG | awk -F\' '{print $2}'`
if [ ! -e $MYRESOURCEDIR ] ; then
    mkdir -p $MYRESOURCEDIR
    chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $MYRESOURCEDIR
fi

# setup database script or resource server script
if [ -e "$DETECTEDDIR/setup_irods_database.sh" ] ; then
  sudo su - $IRODS_SERVICE_ACCOUNT_NAME -c "ORACLE_HOME=$ORACLE_HOME; $DETECTEDDIR/setup_irods_database.sh"
else
  if [ -e "$DETECTEDDIR/setup_resource.sh" ] ; then
    sudo su - $IRODS_SERVICE_ACCOUNT_NAME -c "$DETECTEDDIR/setup_resource.sh"
  else
    echo "" 1>&2
    echo "ERROR:" 1>&2
    echo "  Please install an iRODS Database Plugin" 1>&2
    echo "  and re-run ${BASH_SOURCE[0]}" 1>&2
    exit 1
  fi
fi
