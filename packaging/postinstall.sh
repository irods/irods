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
    IRODS_SERVICE_ACCOUNT_NAME=`stat --format "%U" /var/lib/irods`
    IRODS_SERVICE_GROUP_NAME=`stat --format "%G" /var/lib/irods`
fi

# =-=-=-=-=-=-=-
# set upgrade perms and display helpful information
# make sure the service account owns everything
# careful not to stomp msiExecCmd_bin contents (perhaps managed by others)
chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME /etc/irods
chown -H -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/clients
chown -H -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/log
chown -H -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/packaging
chown -H -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/scripts
chown -H -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/test
chown -H -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/config
chown -H -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/configuration_schemas
chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME
chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/irodsctl
chown -f $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/VERSION*
chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/version*
chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/msiExecCmd_bin/test_execstream.py
chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/msiExecCmd_bin/univMSSInterface.sh.template
chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/msiExecCmd_bin/irodsServerMonPerf
chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/msiExecCmd_bin/hello

# =-=-=-=-=-=-=-
# set permission on single testfile to a probably unresolvable uid
touch /tmp/irods_unresolvable_uid_testfile__issue_4040
chown 29999:29999 /tmp/irods_unresolvable_uid_testfile__issue_4040

if [ ! $UPGRADE_FLAG ] ; then
    cat $IRODS_HOME/packaging/server_setup_instructions.txt
fi

# =-=-=-=-=-=-=-
# remove temporary files
rm -f $UPGRADE_FLAG_FILE

ldconfig

# =-=-=-=-=-=-=-
# exit with success
exit 0
