#!/bin/bash -e

########################################
# Prompt for Service Account Information
########################################

echo "==================================================================="
echo ""
echo "The iRODS service account name needs to be defined."
echo ""

# get service account name
echo -n "iRODS service account name [irods]: "
read MYACCTNAME
if [ "$MYACCTNAME" == "" ] ; then
  MYACCTNAME="irods"
fi
# strip all forward slashes
MYACCTNAME=`echo "${MYACCTNAME}" | sed -e "s/\///g"`
echo ""

# get group name
echo -n "iRODS service group name [irods]: "
read MYGROUPNAME
if [ "$MYGROUPNAME" == "" ] ; then
  MYGROUPNAME="irods"
fi
# strip all forward slashes
MYGROUPNAME=`echo "${MYGROUPNAME}" | sed -e "s/\///g"`
echo ""

echo "==================================================================="

##################################
# Set up Service Group and Account
##################################

SERVICE_ACCOUNT_CONFIG_FILE="/etc/irods/service_account.config"
IRODS_HOME_DIR="/var/lib/irods"

# Group
set +e
CHECKGROUP=`getent group $MYGROUPNAME `
set -e
if [ "$CHECKGROUP" = "" ] ; then
  # new group
  echo "Creating Service Group: $MYGROUPNAME "
  /usr/sbin/groupadd -r $MYGROUPNAME
else
  # use existing group
  echo "Existing Group Detected: $MYGROUPNAME "
fi

# Account
set +e
CHECKACCT=`getent passwd $MYACCTNAME `
set -e
SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
DETECTEDOS=`$SCRIPTPATH/find_os.sh`
if [ "$CHECKACCT" = "" ] ; then
  # new account
  echo "Creating Service Account: $MYACCTNAME at $IRODS_HOME_DIR "
  /usr/sbin/useradd -r -d $IRODS_HOME_DIR -M -s /bin/bash -g $MYGROUPNAME -c "iRODS Administrator" $MYACCTNAME
  # lock the service account
  if [ "$DETECTEDOS" != "MacOSX" ] ; then
    passwd -l $MYACCTNAME > /dev/null
  fi
else
  # use existing account
  # leave user settings and files as is
  echo "Existing Account Detected: $MYACCTNAME "
fi

# Save to config file
mkdir -p /etc/irods
echo "IRODS_SERVICE_ACCOUNT_NAME=$MYACCTNAME " > $SERVICE_ACCOUNT_CONFIG_FILE
echo "IRODS_SERVICE_GROUP_NAME=$MYGROUPNAME " >> $SERVICE_ACCOUNT_CONFIG_FILE

#############
# Permissions
#############
mkdir -p $IRODS_HOME_DIR
chown -R $MYACCTNAME:$MYGROUPNAME $IRODS_HOME_DIR
chown -R $MYACCTNAME:$MYGROUPNAME /etc/irods

# =-=-=-=-=-=-=-
# set permissions on iRODS authentication mechanisms
if [ "$DETECTEDOS" == "MacOSX" ] ; then
    chown root:wheel $IRODS_HOME_DIR/iRODS/server/bin/PamAuthCheck
else
    chown root:root $IRODS_HOME_DIR/iRODS/server/bin/PamAuthCheck
fi
chmod 4755 $IRODS_HOME_DIR/iRODS/server/bin/PamAuthCheck
chmod 4755 /usr/bin/genOSAuth
