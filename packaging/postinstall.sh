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
# wrap previous VERSION.json with new VERSION.json
if [ "$UPGRADE_FLAG" == "true" ] ; then
    python -c "from __future__ import print_function; import json; data=json.load(open('$IRODS_HOME/VERSION.json')); data['previous_version'] = json.load(open('$IRODS_HOME/VERSION.json.previous')); print(json.dumps(data, indent=4, sort_keys=True))" > $IRODS_HOME/VERSION.json.tmp
    mv $IRODS_HOME/VERSION.json.tmp $IRODS_HOME/VERSION.json
    rm $IRODS_HOME/VERSION.json.previous
fi

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
# display helpful information
if [ "$UPGRADE_FLAG" == "true" ] ; then
    # get service account information
    source /etc/irods/service_account.config 2> /dev/null

    # make sure the service acount owns everything except the PAM executable, once again
    chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME /etc/irods
    chown -R $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $IRODS_HOME
    chmod 4755 /usr/bin/genOSAuth


    # convert any legacy configuration files
    su - $IRODS_SERVICE_ACCOUNT_NAME -c "python $IRODS_HOME/packaging/convert_configuration_to_json.py $SERVER_TYPE"
    # update the configuration files
    su - $IRODS_SERVICE_ACCOUNT_NAME -c "python $IRODS_HOME/packaging/update_configuration_schema.py"

    # stop server
    su - $IRODS_SERVICE_ACCOUNT_NAME -c "$IRODS_HOME/irodsctl stop"
    # update the database schema if an icat server
    if [ "$SERVER_TYPE" == "icat" ] ; then
        # =-=-=-=-=-=-=-
        # update the catalog schema
        su - $IRODS_SERVICE_ACCOUNT_NAME -c "python $IRODS_HOME/scripts/python/setup_irods.py update_catalog_schema"
    fi
    # re-start server
    su - $IRODS_SERVICE_ACCOUNT_NAME -c "$IRODS_HOME/irodsctl start"
else
    # copy packaged template files into live config directory
    cp /var/lib/irods/packaging/core.re.template /etc/irods/core.re
    cp /var/lib/irods/packaging/core.dvm.template /etc/irods/core.dvm
    cp /var/lib/irods/packaging/core.fnm.template /etc/irods/core.fnm
    cp /var/lib/irods/packaging/server_config.json.template /etc/irods/server_config.json
    cp /var/lib/irods/packaging/hosts_config.json.template /etc/irods/hosts_config.json
    cp /var/lib/irods/packaging/host_access_control_config.json.template /etc/irods/host_access_control_config.json

    # =-=-=-=-=-=-=-
    if [ "$SERVER_TYPE" == "icat" ] ; then
        # copy packaged template database file into live config directory
        cp /var/lib/irods/packaging/database_config.json.template /etc/irods/database_config.json
        # give user some guidance regarding database selection and installation
        cat $IRODS_HOME/packaging/user_icat.txt
    elif [ "$SERVER_TYPE" == "resource" ] ; then
        # give user some guidance regarding resource configuration
        cat $IRODS_HOME/packaging/user_resource.txt
    fi
fi

# =-=-=-=-=-=-=-
# remove temporary files
rm -f $IRODS_HOME/plaintext_database_password.txt
rm -f $UPGRADE_FLAG_FILE

# =-=-=-=-=-=-=-
# chown the binary_installation.flag file
chown $IRODS_SERVICE_ACCOUNT_NAME:$IRODS_SERVICE_GROUP_NAME $BINARY_INSTALL_FLAG_FILE

# =-=-=-=-=-=-=-
# exit with success
exit 0
