#!/bin/bash -e

EIRODS_HOME_DIR=$1
OS_EIRODS_ACCT=$2
SERVER_TYPE=$3
DB_TYPE=$4
DB_ADMIN_ROLE=$5
DB_NAME=$6
DB_HOST=$7
DB_PORT=$8
DB_USER=$9

IRODS_HOME=$EIRODS_HOME_DIR/iRODS

# =-=-=-=-=-=-=-
# detect whether this is an upgrade
UPGRADE_FLAG_FILE=$EIRODS_HOME_DIR/upgrade.tmp
if [ -f "$UPGRADE_FLAG_FILE" ] ; then
    UPGRADE_FLAG="true"
else
    UPGRADE_FLAG="false"
fi
rm -f $UPGRADE_FLAG_FILE

# =-=-=-=-=-=-=-
# database password
if [ "$UPGRADE_FLAG" == "true" ] ; then
    # read existing database password
    DB_PASS=`grep DATABASE_ADMIN_PASSWORD $EIRODS_HOME_DIR/iRODS/config/irods.config | awk -F\' '{print $2}'`
    DB_PASS=`echo $DB_PASS | tr -d ' '`
else
    # generate new database password
    DB_PASS=`cat /dev/urandom | base64 | head -c16`
fi
#echo "DB_PASS=[$DB_PASS]"
#echo "UPGRADE_FLAG=[$UPGRADE_FLAG]"
#echo "EIRODS_HOME_DIR=$EIRODS_HOME_DIR"
#echo "OS_EIRODS_ACCT=$OS_EIRODS_ACCT"
#echo "SERVER_TYPE=$SERVER_TYPE"
#echo "DB_TYPE=$DB_TYPE"
#echo "DB_ADMIN_ROLE=$DB_ADMIN_ROLE"
#echo "DB_NAME=$DB_NAME"
#echo "DB_HOST=$DB_HOST"
#echo "DB_PORT=$DB_PORT"
#echo "DB_USER=$DB_USER"
#echo "DB_PASS=$DB_PASS"


# =-=-=-=-=-=-=-
# detect operating system
DETECTEDOS=`$EIRODS_HOME_DIR/packaging/find_os.sh`

# =-=-=-=-=-=-=-
# clean up any stray iRODS files in /tmp which will cause problems
if [ -f /tmp/irodsServer.* ]; then
  rm /tmp/irodsServer.*
fi

# =-=-=-=-=-=-=-
# explode tarball of necessary coverage files if it exists
if [ -f "$EIRODS_HOME_DIR/gcovfiles.tgz" ] ; then
    cd $EIRODS_HOME_DIR
    tar xzf gcovfiles.tgz
fi

if [ "$SERVER_TYPE" == "icat" ] ; then

  if [ "$DB_TYPE" == "postgres" ] ; then
    # =-=-=-=-=-=-=-
    # trap possible errors where db, user, etc exist before making any
    # changes to the system

    # =-=-=-=-=-=-=-
    # detect database path and update installed irods.config accordingly
    PGPATH=`$EIRODS_HOME_DIR/packaging/find_postgres_bin.sh`
	if [ $PGPATH == "FAIL" ]; then
		echo "ERROR :: Multiple Versions of postgres found, Aborting."
		echo `find /usr -name "psql" -print 2> /dev/null`
		exit 1
	fi

    # =-=-=-=-=-=-=-
    # build the proper path to psql
    PSQL="$PGPATH/psql"

    # =-=-=-=-=-=-=-
    # make sure postgres is running on this machine
    PSQLSTATUS="notrunning"
    if [ "$DETECTEDOS" == "SuSE" ] ; then
        PSQLSTATE=`/etc/init.d/postgresql status 2>&1 | grep "Active" | awk '{print $2}'`
        if [ "$PSQLSTATE" == "active" ] ; then
            PSQLSTATUS="running"
        else
            PSQLSTATE=`/etc/init.d/postgresql status 2>&1 | grep "running" | awk '{print $4}'`
            if [ "$PSQLSTATE" == "..running" ] ; then
                PSQLSTATUS="running"
            fi
        fi
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PSQLSTATE=`/etc/init.d/postgresql status 2>&1 | grep "postmaster" | awk '{print $NF}'`
        if [ "$PSQLSTATE" == "running..." ] ; then
            PSQLSTATUS="running"
        else
            PSQLSTATE=`service postgresql status 2>&1 | grep "running"`
            if [ "$PSQLSTATE" != "" ] ; then
                PSQLSTATUS="running"
            fi
        fi
    elif [ "$DETECTEDOS" == "Ubuntu" ] ; then
        PSQLSTATE=`/etc/init.d/postgresql-8.4 status 2>&1 | grep "clusters" | awk '{print $3}'`
        if [ "$PSQLSTATE" != "" ] ; then
            PSQLSTATUS="running"
        else
            PSQLSTATE=`/etc/init.d/postgresql status 2>&1 | grep "clusters" | awk '{print $3}'`
            if [ "$PSQLSTATE" != "" ] ; then
                PSQLSTATUS="running"
            else
                PSQLSTATE=`/etc/init.d/postgresql status 2>&1 | grep "online" | awk '{print $4}'`
                if [ "$PSQLSTATE" != "" ] ; then
                    PSQLSTATUS="running"
                fi
            fi
        fi
    else
        PSQLSTATUS="running"
    fi
    if [ "$PSQLSTATUS" != "running" ] ; then
        echo "ERROR :: Installed PostgreSQL database server needs to be running, Aborting."
        if [ "$DETECTEDOS" == "SuSE" ] ; then
            echo "      :: try:"
            echo "      ::      sudo /usr/sbin/rcpostgresql start"
        elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
            echo "      :: try:"
            echo "      ::      sudo service postgresql initdb"
            echo "      ::      sudo service postgresql start"
        fi
        exit 1
    fi

    # =-=-=-=-=-=-=-
    # determine if the database already exists
    set +e
    DB=$( su --shell=/bin/bash -c "$PSQL --list" $DB_ADMIN_ROLE  | grep $DB_NAME )
    set -e
    if [ -n "$DB" -a "$UPGRADE_FLAG" == "false" ] ; then
      echo "ERROR :: Database $DB_NAME Already Exists, Aborting."
	  exit 1
    fi

    if [ "$UPGRADE_FLAG" == "false" ] ; then
        # =-=-=-=-=-=-=-
        # find postgres path & psql - modify config/irods.config accordingly
        EIRODSPOSTGRESDIR="$PGPATH/"
        echo "Detecting PostgreSQL Path: [$EIRODSPOSTGRESDIR]"
        sed -e "\,^\$DATABASE_HOME,s,^.*$,\$DATABASE_HOME = '$EIRODSPOSTGRESDIR';," $IRODS_HOME/config/irods.config > /tmp/irods.config.tmp
        mv /tmp/irods.config.tmp $IRODS_HOME/config/irods.config

        # =-=-=-=-=-=-=-
        # update config/irods.config with new generated password
        sed -e "s,TEMPLATE_DB_PASS,$DB_PASS," $IRODS_HOME/config/irods.config > /tmp/irods.config.tmp
        mv /tmp/irods.config.tmp $IRODS_HOME/config/irods.config

        # =-=-=-=-=-=-=-
        # determine if the database role already exists
        ROLE=$( su --shell=/bin/bash -c "$PSQL $DB_ADMIN_ROLE -tAc \"SELECT 1 FROM pg_roles WHERE rolname='$DB_USER'\"" $DB_ADMIN_ROLE )
        if [ $ROLE ]; then
            echo "ERROR :: Role $DB_USER Already Exists in Database, Aborting."
            exit 1
        fi

        # =-=-=-=-=-=-=-
        # create the database role
        echo "Creating Database Role: $DB_USER as $DB_ADMIN_ROLE"
        su --shell=/bin/bash -c "createuser -s $DB_USER" $DB_ADMIN_ROLE &> /dev/null

        # =-=-=-=-=-=-=-
        # update new role with proper password
        echo "Updating Database Role Password..."
        ALTERPASSCMD="alter user $DB_USER with password '$DB_PASS'"
        su --shell=/bin/bash -c "$PSQL -c \"$ALTERPASSCMD\"" $DB_ADMIN_ROLE &> /dev/null

        # =-=-=-=-=-=-=-
        # create the database
        echo "Creating Database: $DB_NAME as $DB_USER"
        su --shell=/bin/bash -c "createdb $DB_NAME" $DB_USER &> /dev/null
    fi

  else
    # =-=-=-=-=-=-=-
    # catch other database types
    echo "TODO: detect location of non-postgres database"
    echo "TODO: create database role"
    echo "TODO: check for existing database"
  fi



fi

# =-=-=-=-=-=-=-
# set permissions on the installed files
chown -R $OS_EIRODS_ACCT:$OS_EIRODS_ACCT $IRODS_HOME

# =-=-=-=-=-=-=-
# touch odbc file so it exists for the install script to update
touch $EIRODS_HOME_DIR/.odbc.ini
chown $OS_EIRODS_ACCT:$OS_EIRODS_ACCT $EIRODS_HOME_DIR/.odbc.ini


# =-=-=-=-=-=-=-
# setup runlevels and aliases (use os-specific tools)
if [ "$DETECTEDOS" == "Ubuntu" ] ; then
    update-rc.d eirods defaults
elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
    /sbin/chkconfig --add eirods
elif [ "$DETECTEDOS" == "SuSE" ] ; then
    /sbin/chkconfig --add eirods
fi




# =-=-=-=-=-=-=-
# symlink the icommands

#ln -s    /usr/bin/chgCoreToCore1.ir       ${IRODS_HOME}/clients/icommands/bin/chgCoreToCore1.ir 
#ln -s    /usr/bin/chgCoreToCore2.ir       ${IRODS_HOME}/clients/icommands/bin/chgCoreToCore2.ir 
#ln -s    /usr/bin/chgCoreToOrig.ir        ${IRODS_HOME}/clients/icommands/bin/chgCoreToOrig.ir  
#ln -s    /usr/bin/delUnusedAVUs.ir        ${IRODS_HOME}/clients/icommands/bin/delUnusedAVUs.ir  
#ln -s    /usr/bin/runQuota.ir             ${IRODS_HOME}/clients/icommands/bin/runQuota.ir       
#ln -s    /usr/bin/runQuota.r              ${IRODS_HOME}/clients/icommands/bin/runQuota.r        
#ln -s    /usr/bin/showCore.ir             ${IRODS_HOME}/clients/icommands/bin/showCore.ir       

ln -fs    /usr/bin/genOSAuth               ${IRODS_HOME}/clients/icommands/bin/genOSAuth         
ln -fs    /usr/bin/iadmin                  ${IRODS_HOME}/clients/icommands/bin/iadmin            
ln -fs    /usr/bin/ibun                    ${IRODS_HOME}/clients/icommands/bin/ibun              
ln -fs    /usr/bin/icd                     ${IRODS_HOME}/clients/icommands/bin/icd               
ln -fs    /usr/bin/ichksum                 ${IRODS_HOME}/clients/icommands/bin/ichksum           
ln -fs    /usr/bin/ichmod                  ${IRODS_HOME}/clients/icommands/bin/ichmod            
ln -fs    /usr/bin/icp                     ${IRODS_HOME}/clients/icommands/bin/icp               
ln -fs    /usr/bin/idbug                   ${IRODS_HOME}/clients/icommands/bin/idbug             
ln -fs    /usr/bin/ienv                    ${IRODS_HOME}/clients/icommands/bin/ienv              
ln -fs    /usr/bin/ierror                  ${IRODS_HOME}/clients/icommands/bin/ierror            
ln -fs    /usr/bin/iexecmd                 ${IRODS_HOME}/clients/icommands/bin/iexecmd           
ln -fs    /usr/bin/iexit                   ${IRODS_HOME}/clients/icommands/bin/iexit             
ln -fs    /usr/bin/ifsck                   ${IRODS_HOME}/clients/icommands/bin/ifsck             
ln -fs    /usr/bin/iget                    ${IRODS_HOME}/clients/icommands/bin/iget              
ln -fs    /usr/bin/igetwild                ${IRODS_HOME}/clients/icommands/bin/igetwild          
ln -fs    /usr/bin/igroupadmin             ${IRODS_HOME}/clients/icommands/bin/igroupadmin       
ln -fs    /usr/bin/ihelp                   ${IRODS_HOME}/clients/icommands/bin/ihelp             
ln -fs    /usr/bin/iinit                   ${IRODS_HOME}/clients/icommands/bin/iinit             
ln -fs    /usr/bin/ilocate                 ${IRODS_HOME}/clients/icommands/bin/ilocate           
ln -fs    /usr/bin/ils                     ${IRODS_HOME}/clients/icommands/bin/ils               
ln -fs    /usr/bin/ilsresc                 ${IRODS_HOME}/clients/icommands/bin/ilsresc           
ln -fs    /usr/bin/imcoll                  ${IRODS_HOME}/clients/icommands/bin/imcoll            
ln -fs    /usr/bin/imeta                   ${IRODS_HOME}/clients/icommands/bin/imeta             
ln -fs    /usr/bin/imiscsvrinfo            ${IRODS_HOME}/clients/icommands/bin/imiscsvrinfo      
ln -fs    /usr/bin/imkdir                  ${IRODS_HOME}/clients/icommands/bin/imkdir            
ln -fs    /usr/bin/imv                     ${IRODS_HOME}/clients/icommands/bin/imv               
ln -fs    /usr/bin/ipasswd                 ${IRODS_HOME}/clients/icommands/bin/ipasswd           
ln -fs    /usr/bin/iphybun                 ${IRODS_HOME}/clients/icommands/bin/iphybun           
ln -fs    /usr/bin/iphymv                  ${IRODS_HOME}/clients/icommands/bin/iphymv            
ln -fs    /usr/bin/ips                     ${IRODS_HOME}/clients/icommands/bin/ips               
ln -fs    /usr/bin/iput                    ${IRODS_HOME}/clients/icommands/bin/iput              
ln -fs    /usr/bin/ipwd                    ${IRODS_HOME}/clients/icommands/bin/ipwd              
ln -fs    /usr/bin/iqdel                   ${IRODS_HOME}/clients/icommands/bin/iqdel             
ln -fs    /usr/bin/iqmod                   ${IRODS_HOME}/clients/icommands/bin/iqmod             
ln -fs    /usr/bin/iqstat                  ${IRODS_HOME}/clients/icommands/bin/iqstat            
ln -fs    /usr/bin/iquest                  ${IRODS_HOME}/clients/icommands/bin/iquest            
ln -fs    /usr/bin/iquota                  ${IRODS_HOME}/clients/icommands/bin/iquota            
ln -fs    /usr/bin/ireg                    ${IRODS_HOME}/clients/icommands/bin/ireg              
ln -fs    /usr/bin/irepl                   ${IRODS_HOME}/clients/icommands/bin/irepl             
ln -fs    /usr/bin/irm                     ${IRODS_HOME}/clients/icommands/bin/irm               
ln -fs    /usr/bin/irmtrash                ${IRODS_HOME}/clients/icommands/bin/irmtrash          
ln -fs    /usr/bin/irsync                  ${IRODS_HOME}/clients/icommands/bin/irsync            
ln -fs    /usr/bin/irule                   ${IRODS_HOME}/clients/icommands/bin/irule             
ln -fs    /usr/bin/iscan                   ${IRODS_HOME}/clients/icommands/bin/iscan             
ln -fs    /usr/bin/isysmeta                ${IRODS_HOME}/clients/icommands/bin/isysmeta          
ln -fs    /usr/bin/itrim                   ${IRODS_HOME}/clients/icommands/bin/itrim             
ln -fs    /usr/bin/iuserinfo               ${IRODS_HOME}/clients/icommands/bin/iuserinfo         
ln -fs    /usr/bin/ixmsg                   ${IRODS_HOME}/clients/icommands/bin/ixmsg             









cd $PWD

if [ "$UPGRADE_FLAG" == "true" ] ; then
    # =-=-=-=-=-=-=-
    # start the upgraded server
    # (instead of running eirods_setup.pl which would have started it)
    set +e
    su --shell=/bin/bash -c "cd $IRODS_HOME; ./irodsctl start" $OS_EIRODS_ACCT
    set -e
else
    # =-=-=-=-=-=-=-
    # run setup script to configure an ICAT server
    if [ "$SERVER_TYPE" == "icat" ] ; then
        cd $IRODS_HOME
        su --shell=/bin/bash -c "perl $IRODS_HOME/scripts/perl/eirods_setup.pl $DB_TYPE $DB_HOST $DB_PORT $DB_USER $DB_PASS" $OS_EIRODS_ACCT
    fi

    # =-=-=-=-=-=-=-
    # remove setup 'rodsBoot' account - reduce potential attack surface
    if [ "$SERVER_TYPE" == "icat" ] ; then
        su --shell=/bin/bash -c "/usr/bin/iadmin rmuser rodsBoot" $OS_EIRODS_ACCT
    fi
fi

# =-=-=-=-=-=-=-
# really make sure everything is owned by the eirods service account
chown -R $OS_EIRODS_ACCT:$OS_EIRODS_ACCT $EIRODS_HOME_DIR

# =-=-=-=-=-=-=-
# set permissions on iRODS authentication mechanisms
chown root:root $IRODS_HOME/server/bin/PamAuthCheck
chmod 4755 $IRODS_HOME/server/bin/PamAuthCheck
chmod 4755 /usr/bin/genOSAuth

# =-=-=-=-=-=-=-
# remove the password from the service account
passwd -d $OS_EIRODS_ACCT


if [ "$UPGRADE_FLAG" == "false" ] ; then
    # =-=-=-=-=-=-=-
    if [ "$SERVER_TYPE" == "icat" ] ; then
        # tell user about their irodsenv
        cat $EIRODS_HOME_DIR/packaging/user_irodsenv.txt
        cat $EIRODS_HOME_DIR/.irods/.irodsEnv
    elif [ "$SERVER_TYPE" == "resource" ] ; then
        # give user some guidance regarding resource configuration
        cat $EIRODS_HOME_DIR/packaging/user_resource.txt
    fi
fi




# =-=-=-=-=-=-=-
# RPM runs old 3.0 uninstall script last, which removed everything
# Detect this case and protect the existing data
if [ "$UPGRADE_FLAG" == "true" -a ! -f $EIRODS_HOME_DIR/packaging/VERSION ] ; then
    # Check for RPM-based systems
    if [ "$DETECTEDOS" == "RedHatCompatible" -o "$DETECTEDOS" == "SuSE" ] ; then
        # stop the running server
        su --shell=/bin/bash -c "cd $IRODS_HOME; ./irodsctl stop" $OS_EIRODS_ACCT
        # detect whether eirods home directory is a mount point
        set +e
        mountpoint $EIRODS_HOME_DIR > /dev/null
        ISMOUNTPOINT=$?
        set -e
        # if a mount point
        if [ $ISMOUNTPOINT -eq 0 ] ; then
            # detect current mounted device
            MOUNTED_DEVICE=`df | grep " $EIRODS_HOME_DIR$" | awk '{print $1}'`
            # unmount eirods home directory
            set +e
            umount $EIRODS_HOME_DIR
            set -e
            # report to the admin what happened
            echo "$EIRODS_HOME_DIR has been unmounted from $MOUNTED_DEVICE"
            echo "Once $EIRODS_HOME_DIR is mounted again, start the server:"
            echo "  sudo su - eirods -c 'cd $IRODS_HOME; ./irodsctl start'"
        else
            # if not, move it aside
            mv $EIRODS_HOME_DIR ${EIRODS_HOME_DIR}_new
            # create a passable directory in its place, which will be deleted by existing 3.0 postun script
            mkdir -p $EIRODS_HOME_DIR
            cp -r ${EIRODS_HOME_DIR}_new/packaging $EIRODS_HOME_DIR/packaging
            mkdir $EIRODS_HOME_DIR/iRODS
            cp -r ${EIRODS_HOME_DIR}_new/iRODS/irodsctl $EIRODS_HOME_DIR/iRODS/irodsctl
            cp -r ${EIRODS_HOME_DIR}_new/iRODS/scripts $EIRODS_HOME_DIR/iRODS/scripts
            mkdir -p $EIRODS_HOME_DIR/iRODS/clients/icommands
            cp -r ${EIRODS_HOME_DIR}_new/iRODS/clients/icommands/bin $EIRODS_HOME_DIR/iRODS/clients/icommands/bin
            # report to the admin what happened
#            echo "$EIRODS_HOME_DIR has been moved to ${EIRODS_HOME_DIR}_new"
#            echo "Please replace it and start the server:"
#            echo "  sudo mv ${EIRODS_HOME_DIR}_new $EIRODS_HOME_DIR"
#            echo "  sudo su - eirods -c 'cd $IRODS_HOME; ./irodsctl start'"
            echo "#########################################################"
            echo "#"
            echo "#  Please run the recovery script:"
            echo "#    sudo ${EIRODS_HOME_DIR}_new/packaging/post30upgrade.sh newfile.rpm"
            echo "#"
            echo "#########################################################"

        fi
    fi
fi




# =-=-=-=-=-=-=-
# exit with success
exit 0

