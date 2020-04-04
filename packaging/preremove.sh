#!/bin/bash

IRODS_HOME=$1
SERVER_TYPE=$2

#echo "parameter count = [$#]"
if [ $# -gt 7 ] ; then
    # new icat
    THREE_OH_SCRIPT="false"
    PACKAGER_COMMAND=$1
    shift
elif [ $# -eq 7 ] ; then
    # old icat
    THREE_OH_SCRIPT="true"
    PACKAGER_COMMAND="upgrade"
elif [ $# -eq 3 ] ; then
    # new resource
    PACKAGER_COMMAND=$1
    shift
elif [ $# -eq 2 ] ; then
    # old resource
    PACKAGER_COMMAND="upgrade"
elif [ $# -eq 4 ] ; then
    # upgrade from 4.0.2
    PACKAGER_COMMAND="upgrade"
else
    echo "ERROR: Unspecified state for preremove.sh - [$#]"
    exit 1
fi

if [ -f /etc/irods/service_account.config ] ; then
    # get service account information
    source /etc/irods/service_account.config 2> /dev/null
else
    IRODS_SERVICE_ACCOUNT_NAME=`ls -ld /var/lib/irods | awk '{print $3}'`
    IRODS_SERVICE_GROUP_NAME=`ls -ld /var/lib/irods | awk '{print $4}'`
fi

# =-=-=-=-=-=-=-
# debugging
#echo "THREE_OH_SCRIPT=[$THREE_OH_SCRIPT]"
#echo "PACKAGER_COMMAND=[$PACKAGER_COMMAND]"
#echo "IRODS_HOME=[$IRODS_HOME]"
#echo "SERVER_TYPE=[$SERVER_TYPE]"
#echo "IRODS_SERVICE_ACCOUNT_NAME=[$IRODS_SERVICE_ACCOUNT_NAME]"
#echo "IRODS_SERVICE_GROUP_NAME=[$IRODS_SERVICE_GROUP_NAME]"

# determine whether this is an upgrade
if [ "$PACKAGER_COMMAND" -eq "$PACKAGER_COMMAND" ] 2>/dev/null ; then
    # integer, therefore rpm
    if [ $PACKAGER_COMMAND -gt 0 ] ; then
        PACKAGEUPGRADE="true"
    else
        PACKAGEUPGRADE="false"
    fi
else
    # string, therefore deb
    if [ "$PACKAGER_COMMAND" = "upgrade" ] ; then
        PACKAGEUPGRADE="true"
    else
        PACKAGEUPGRADE="false"
    fi
fi

#echo "PACKAGEUPGRADE=[$PACKAGEUPGRADE]"

if [ "$PACKAGEUPGRADE" == "false" ] ; then

    # =-=-=-=-=-=-=-
    # stop any running iRODS Processes
    echo "Stopping iRODS :: $IRODS_HOME/irodsctl stop"
    cd $IRODS_HOME
    su --shell=/bin/bash -c "$IRODS_HOME/irodsctl stop" $IRODS_SERVICE_ACCOUNT_NAME
    cd /tmp

    # =-=-=-=-=-=-=-
    # detect operating system
    DETECTEDOS=`bash $IRODS_HOME/packaging/find_os.sh`

    # =-=-=-=-=-=-=-
    # report that we are not deleting some things
    echo "NOTE :: The Local System Administrator should delete these if necessary."

    # =-=-=-=-=-=-=-
    # report that we are not deleting the account(s)
    echo "     :: Leaving $IRODS_SERVICE_ACCOUNT_NAME Service Account in place."
    if [ "$DETECTEDOS" == "RedHatCompatible" ]; then # CentOS and RHEL and Fedora
        echo "     :: try:"
        echo "     ::      sudo /usr/sbin/userdel $IRODS_SERVICE_ACCOUNT_NAME"
    elif [ "$DETECTEDOS" == "SuSE" ]; then # SuSE
        echo "     :: try:"
        echo "     ::      sudo /usr/sbin/userdel $IRODS_SERVICE_ACCOUNT_NAME"
        echo "     ::      sudo /usr/sbin/groupdel $IRODS_SERVICE_GROUP_NAME"
    elif [ "$DETECTEDOS" == "Ubuntu" ]; then  # Ubuntu
        echo "     :: try:"
        echo "     ::      sudo userdel $IRODS_SERVICE_ACCOUNT_NAME"
                           # groupdel is not necessary on Ubuntu, apparently...
    fi

    # =-=-=-=-=-=-=-
    # remove runlevels and aliases (use os-specific tools)
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then
        update-rc.d -f irods remove
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        /sbin/chkconfig --del irods
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        /sbin/chkconfig --del irods
    fi

fi

# =-=-=-=-=-=-=-
# exit with success
exit 0
