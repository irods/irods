#!/bin/bash

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
elif [ $# -eq 4 ] ; then
  # new resource
  PACKAGER_COMMAND=$1
  shift
elif [ $# -eq 3 ] ; then
  # old resource
  PACKAGER_COMMAND="upgrade"
else
  echo "ERROR: Unspecified state for preremove.sh"
  exit 1
fi

IRODS_HOME_DIR=$1
OS_IRODS_ACCT=$2
SERVER_TYPE=$3

IRODS_HOME=$IRODS_HOME_DIR/iRODS

# =-=-=-=-=-=-=-
# debugging
#echo "THREE_OH_SCRIPT=[$THREE_OH_SCRIPT]"
#echo "PACKAGER_COMMAND=[$PACKAGER_COMMAND]"
#echo "IRODS_HOME_DIR=[$IRODS_HOME_DIR]"
#echo "OS_IRODS_ACCT=[$OS_IRODS_ACCT]"
#echo "SERVER_TYPE=[$SERVER_TYPE]"


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
	# determine if we can delete the service account
	user=`who | grep $OS_IRODS_ACCT`
	if [ "x$user" != "x" ]; then
		echo "ERROR :: $OS_IRODS_ACCT is currently logged in.  Aborting."
		exit 1
	fi

	# =-=-=-=-=-=-=-
	# determine if we can remove the resource from the zone
	if [ "$SERVER_TYPE" == "resource" ] ; then

	    hn=`hostname`
	    dn=`hostname -d`
	    fhn="$hn.$dn"
	    echo "Testing Safe Resource Removal"
	    do_not_remove="FALSE"

	    # =-=-=-=-=-=-=-
	    # do a dryrun on the resource removal to determine if this resource server can
	    # be safely removed without harming any data
            resources_to_remove=()
	    for resc in `su -c "iadmin lr" $OS_IRODS_ACCT`
	    do
		# =-=-=-=-=-=-=-
		# for each resource determine its location.  if it is this server then dryrun
		loc=$( su -c "iadmin lr $resc | grep resc_net | cut -d' ' -f2" $OS_IRODS_ACCT )

		if [[ $loc == $hn || $loc == $fhn ]]; then
			rem=$( su -c "iadmin rmresc --dryrun $resc | grep SUCCESS" $OS_IRODS_ACCT )
			if [[ "x$rem" == "x" ]]; then
				# =-=-=-=-=-=-=-
                                # dryrun for a local resource was a failure, set a flag
                                do_not_remove="TRUE"
                        else
                                # =-=-=-=-=-=-=-
				# dryrun for a local resource was a success, add resc to array for removal
                                echo "  Adding [$resc] for removal..."
				resources_to_remove+=($resc)
			fi
		fi
	    done

	    if [[ "$do_not_remove" == "TRUE" ]]; then
                # hard stop
		echo "ERROR :: Unable To Remove a Locally Resident Resource.  Aborting."
		echo "      :: Please run 'iadmin rmresc --dryrun RESOURCE_NAME' on local resources for more information."
		exit 1
            else
                # loop through and remove the resources
                for delresc in ${resources_to_remove[*]}
                do
                    echo "  Removing Resource [$delresc]"
                    su -c "iadmin rmresc $delresc" $OS_IRODS_ACCT
                    if [ $? != 0 ] ; then
                        exit 1
                    fi
                done
	    fi
	fi

	# =-=-=-=-=-=-=-
	# stop any running iRODS Processes
	echo "Stopping iRODS :: $IRODS_HOME/irodsctl stop"
	cd $IRODS_HOME
	su --shell=/bin/bash -c "$IRODS_HOME/irodsctl stop" $OS_IRODS_ACCT
	cd /tmp

	# =-=-=-=-=-=-=-
	# detect operating system
	DETECTEDOS=`$IRODS_HOME_DIR/packaging/find_os.sh`

	# =-=-=-=-=-=-=-
	# report that we are not deleting some things
	echo "NOTE :: The Local System Administrator should delete these if necessary."

	# =-=-=-=-=-=-=-
	# report that we are not deleting the account(s)
	echo "     :: Leaving $OS_IRODS_ACCT Service Group and Account in place."
	if [ "$DETECTEDOS" == "RedHatCompatible" ]; then # CentOS and RHEL and Fedora
	    echo "     :: try:"
	    echo "     ::      sudo /usr/sbin/userdel $OS_IRODS_ACCT"
	elif [ "$DETECTEDOS" == "SuSE" ]; then # SuSE
	    echo "     :: try:"
	    echo "     ::      sudo /usr/sbin/userdel $OS_IRODS_ACCT"
	    echo "     ::      sudo /usr/sbin/groupdel $OS_IRODS_ACCT"
	elif [ "$DETECTEDOS" == "Ubuntu" ]; then  # Ubuntu
	    echo "     :: try:"
	    echo "     ::      sudo userdel $OS_IRODS_ACCT"
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

	# =-=-=-=-=-=-=-
	# remove icommands symlinks
	for file in `ls ${IRODS_HOME}/clients/icommands/bin/`
	do
		if [ -e /usr/bin/$file ]; then
	        rm /usr/bin/$file
		fi
	done

fi

# =-=-=-=-=-=-=-
# exit with success
exit 0
