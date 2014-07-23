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

# setup database script or resource server script
if [ -e "$DETECTEDDIR/setup_irods_database.sh" ] ; then
  sudo su - $IRODS_SERVICE_ACCOUNT_NAME -c "$DETECTEDDIR/setup_irods_database.sh"
else
  sudo su - $IRODS_SERVICE_ACCOUNT_NAME -c "$DETECTEDDIR/setup_resource.sh"
fi