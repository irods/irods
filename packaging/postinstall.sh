#!/bin/bash -e

IRODS_HOME=$1
SERVER_TYPE=$2

# =-=-=-=-=-=-=-
# touch the binary_installation.flag file
BINARY_INSTALL_FLAG_FILE=$IRODS_HOME/packaging/binary_installation.flag
touch $BINARY_INSTALL_FLAG_FILE

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
#echo "SERVER_TYPE=[$SERVER_TYPE]"

# =-=-=-=-=-=-=-
# detect operating system
DETECTEDOS=`bash $IRODS_HOME/packaging/find_os.sh`

# get database password from pre-4.1 icat installations
if [ "$UPGRADE_FLAG" == "true" ] && [ "$SERVER_TYPE" == "icat" ] ; then
  MYACCTNAME=`ls -l /etc/irods/core.re | awk '{print $3}'`
  if [ ! -f /etc/irods/database_config.json ] ; then
    DSPASS_INPUT_FILE=`su - $MYACCTNAME -c 'mktemp -t dspass_input.XXXXXX'`
    grep "^DBPassword" /etc/irods/server.config | tail -n1 | awk '{print $2}' > $DSPASS_INPUT_FILE
    grep "^DBKey" /etc/irods/server.config | tail -n1 | awk '{print $2}' >> $DSPASS_INPUT_FILE
    PLAINTEXT_FILENAME=$IRODS_HOME/plaintext_database_password.txt
    rm -f $PLAINTEXT_FILENAME
    touch $PLAINTEXT_FILENAME
    chown $MYACCTNAME:$MYGROUPNAME $PLAINTEXT_FILENAME
    chmod 600 $PLAINTEXT_FILENAME
    su - $MYACCTNAME -c "iadmin dspass < '$DSPASS_INPUT_FILE'" | sed 's/[^:]*://' > $PLAINTEXT_FILENAME
    rm -f $DSPASS_INPUT_FILE
  fi
fi

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
# setup runlevels and aliases (use os-specific tools)
if [ "$DETECTEDOS" == "Ubuntu" ] ; then
    update-rc.d irods defaults
elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
    /sbin/chkconfig --add irods
elif [ "$DETECTEDOS" == "SuSE" ] ; then
    /sbin/chkconfig --add irods
fi

# =-=-=-=-=-=-=-
# set upgrade perms and display helpful information
if [ -f /etc/irods/service_account.config ] ; then
    # get service account information
    source /etc/irods/service_account.config 2> /dev/null
    # make sure the service acount owns everything
    # careful not to stomp server/bin/cmd contents (perhaps managed by others)
    chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME /etc/irods
    chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/clients
    chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/log
    chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/packaging
    chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/plugins
    chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/scripts
    chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/server/config
    chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/test
    chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/irodsctl
    chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME/VERSION.json
    # set PAM and OSAuth executables with setuid bit
    chown root:root /usr/sbin/irodsPamAuthCheck
    chmod 4755 /usr/sbin/irodsPamAuthCheck
    chmod 4755 /usr/bin/genOSAuth
else
    cat $IRODS_HOME/packaging/server_setup_instructions.txt
fi

# =-=-=-=-=-=-=-
# remove temporary files
rm -f $IRODS_HOME/plaintext_database_password.txt
rm -f $UPGRADE_FLAG_FILE

# =-=-=-=-=-=-=-
# exit with success
exit 0
