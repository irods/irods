#!/bin/bash -e

IRODS_HOME=$1

# =-=-=-=-=-=-=-
# detect whether this is an upgrade
UPGRADE_FLAG_FILE=$IRODS_HOME/upgrade.tmp
if [ -f "$UPGRADE_FLAG_FILE" ] ; then
    UPGRADE_FLAG="true"
else
    UPGRADE_FLAG="false"
fi

# =-=-=-=-=-=-=-
# debugging
#echo "UPGRADE_FLAG=[$UPGRADE_FLAG]"
#echo "IRODS_HOME_DIR=[$IRODS_HOME]"

# =-=-=-=-=-=-=-
# clean up any stray iRODS files in /tmp which will cause problems
rm -f /tmp/irodsServer.*

# =-=-=-=-=-=-=-
# clean up any stray iRODS shared memory mutex files
rm -f /var/run/shm/*var_lib_irods_iRODS_server_config*

# =-=-=-=-=-=-=-
# explode tarball of necessary coverage files if it exists
if [ -f "$IRODS_HOME/gcovfiles.tgz" ] ; then
    cd $IRODS_HOME
    tar xzf gcovfiles.tgz
fi

# =-=-=-=-=-=-=-
# detect operating system
DETECTEDOS=`bash $IRODS_HOME/packaging/find_os.sh`

# =-=-=-=-=-=-=-
# setup runlevels and aliases (use os-specific tools)
if [ "$DETECTEDOS" == "Ubuntu" ] ; then
    update-rc.d irods defaults
elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
    /sbin/chkconfig --add irods
elif [ "$DETECTEDOS" == "SuSE" ] ; then
    /sbin/chkconfig --add irods
fi

if [ -f /etc/irods/service_account.config ] ; then
    # get service account information
    source /etc/irods/service_account.config 2> /dev/null
else
    IRODS_SERVICE_ACCOUNT_NAME=`ls -l /var/lib/irods | awk '{print $3}'`
    IRODS_SERVICE_GROUP_NAME=`ls -l /var/lib/irods | awk '{print $4}'`
fi

# =-=-=-=-=-=-=-
# set upgrade perms and display helpful information
# make sure the service acount owns everything
# careful not to stomp msiExecCmd_bin contents (perhaps managed by others)
chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME /etc/irods
chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/clients
chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/log
chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/packaging
chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/plugins
chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/scripts
chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/test
chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/irodsctl
chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/VERSION*
chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/msiExecCmd_bin/test_execstream.py
chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/msiExecCmd_bin/univMSSInterface.sh
chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/msiExecCmd_bin/irodsServerMonPerf
chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/msiExecCmd_bin/hello

if [ ! $UPGRADE_FLAG ] ; then
    cat $IRODS_HOME/packaging/server_setup_instructions.txt
fi

# =-=-=-=-=-=-=-
# remove temporary files
rm -f $UPGRADE_FLAG_FILE

# =-=-=-=-=-=-=-
# exit with success
exit 0
