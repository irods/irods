#!/bin/bash

EIRODS_HOME_DIR=$1
OS_EIRODS_ACCT=$2
SERVER_TYPE=$3
DB_TYPE=$4
DB_ADMIN_ROLE=$5
DB_NAME=$6
DB_USER=$7

IRODS_HOME=$EIRODS_HOME_DIR/iRODS


#echo "EIRODS_HOME_DIR=$EIRODS_HOME_DIR"
#echo "OS_EIRODS_ACCT=$OS_EIRODS_ACCT"
#echo "SERVER_TYPE=$SERVER_TYPE"
#echo "DB_TYPE=$DB_TYPE"
#echo "DB_ADMIN_ROLE=$DB_ADMIN_ROLE"
#echo "DB_NAME=$DB_NAME"
#echo "DB_USER=$DB_USER"

# =-=-=-=-=-=-=-
# determine if we can delete the service account
user=`who | grep $OS_EIRODS_ACCT`
if [ "x$user" != "x" ]; then
    echo "ERROR :: $OS_EIRODS_ACCT is currently logged in.  Aborting."
	exit 1
fi

# =-=-=-=-=-=-=-
# determine if we can remove the resource from the zone
if [ "$SERVER_TYPE" == "resource" ] ; then

    hn=`hostname`
	dn=`hostname -d`

    fhn="$hn.$dn"

	echo "Pre-Remove :: Test Resource Removal"

	do_not_remove="FALSE"

    # =-=-=-=-=-=-=-
    # do a dryrun on the resource removal to determine if this resource server can
    # be safely removed without harming any data
    for resc in `su -c "iadmin lr" $OS_EIRODS_ACCT`
    do
	# =-=-=-=-=-=-=-
	# for each resource determine its location.  if it is this server then dryrun
	loc=$( su -c "iadmin lr $resc | grep resc_net | cut -d' ' -f2" $OS_EIRODS_ACCT )

	if [[ $loc == $hn || $loc == $fhn ]]; then
		rem=$( su -c "iadmin rmresc --dryrun $resc | grep SUCCESS" $OS_EIRODS_ACCT )
		if [[ "x$rem" == "x" ]]; then
			# =-=-=-=-=-=-=-
			# dryrun for a local resource was a success, add resc to array for removal
			do_not_remove="TRUE"
		fi
	fi
    done

    if [[ "$do_not_remove" == "TRUE" ]]; then
		echo "ERROR :: Unable To Remove a Locally Resident Resource.  Aborting."
		echo "      :: Please run 'iadmin rmresc --dryrun RESOURCE_NAME' on local resources for more information."
		exit 1
	fi
fi

# =-=-=-=-=-=-=-
# stop any running E-iRODS Processes
echo "Stopping iRODS :: $IRODS_HOME/irodsctl stop"
cd $IRODS_HOME
su --shell=/bin/bash -c "$IRODS_HOME/irodsctl stop" $OS_EIRODS_ACCT
cd /tmp

# =-=-=-=-=-=-=-
#
#if [ "$SERVER_TYPE" == "icat" ] ; then
#
#	if [ "$DB_TYPE" == "postgres" ] ; then
#        # =-=-=-=-=-=-=-
#        # determine if the database exists & remove
#	    PSQL=`$EIRODS_HOME_DIR/packaging/find_postgres_bin.sh`
#	    PSQL="$PSQL/psql"
#
#	    DB=$( su --shell=/bin/bash -c "$PSQL --list | grep $DB_NAME" $DB_ADMIN_ROLE )
#	    if [ -n "$DB" ]; then
#	        echo "Removing Database $DB_NAME"
#	        su --shell=/bin/bash -c "dropdb $DB_NAME" $DB_ADMIN_ROLE &> /dev/null
#	    fi
#
#        # =-=-=-=-=-=-=-
#        # determine if the database role exists & remove
#	    ROLE=$( su - $OS_EIRODS_ACCT --shell=/bin/bash -c "$PSQL $DB_ADMIN_ROLE -tAc \"SELECT 1 FROM pg_roles WHERE rolname='$DB_USER'\"" )
#	    if [ $ROLE ]; then
#	        echo "Removing Database Role $DB_USER"
#	        su --shell=/bin/bash -c "dropuser $DB_USER" $DB_ADMIN_ROLE &> /dev/null
#	    fi
#	else
#        # expand this for each type of database
#	    echo "TODO: remove non-postgres database"
#	    echo "TODO: remove non-postgres user"
#	fi
#fi

# =-=-=-=-=-=-=-
# detect operating system
DETECTEDOS=`$EIRODS_HOME_DIR/packaging/find_os.sh`

# =-=-=-=-=-=-=-
# report that we are not deleting some things
echo "NOTE :: The Local System Administrator should delete these if necessary."

if [ "$SERVER_TYPE" == "icat" ] ; then
    # =-=-=-=-=-=-=-
    # database(s) and database role
    echo "     :: Leaving the E-iRODS database(s) and role in place."
    echo "     :: try:"
    echo "     ::      sudo su - postgres -c 'dropdb $DB_NAME; dropuser $DB_USER;'"
fi

# =-=-=-=-=-=-=-
# report that we are not deleting the account(s)
echo "     :: Leaving $OS_EIRODS_ACCT Service Group and Account in place."
if [ "$DETECTEDOS" == "RedHatCompatible" ]; then # CentOS and RHEL and Fedora
    echo "     :: try:"
    echo "     ::      sudo /usr/sbin/userdel $OS_EIRODS_ACCT"
elif [ "$DETECTEDOS" == "SuSE" ]; then # SuSE
    echo "     :: try:"
    echo "     ::      sudo /usr/sbin/userdel $OS_EIRODS_ACCT"
    echo "     ::      sudo /usr/sbin/groupdel $OS_EIRODS_ACCT"
elif [ "$DETECTEDOS" == "Ubuntu" ]; then  # Ubuntu
    echo "     :: try:"
    echo "     ::      sudo userdel $OS_EIRODS_ACCT"
                       # groupdel is not necessary on Ubuntu, apparently...
fi

# =-=-=-=-=-=-=-
# remove runlevels and aliases (use os-specific tools)
if [ "$DETECTEDOS" == "Ubuntu" ] ; then
    update-rc.d -f eirods remove
elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
    /sbin/chkconfig --del eirods
elif [ "$DETECTEDOS" == "SuSE" ] ; then
    /sbin/chkconfig --del eirods
fi

# =-=-=-=-=-=-=-
# remove icommands symlinks
for file in `ls ${IRODS_HOME}/clients/icommands/bin/`
do
	if [ -e /usr/bin/$file ]; then
        rm /usr/bin/$file
	fi
done

# =-=-=-=-=-=-=-
# exit with success
exit 0
