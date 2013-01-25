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
DB_PASS=${10}

IRODS_HOME=$EIRODS_HOME_DIR/iRODS


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
        exit 1
    fi

    # =-=-=-=-=-=-=-
    # determine if the database already exists
    set +e
    DB=$( su --shell=/bin/bash -c "$PSQL --list" $DB_ADMIN_ROLE  | grep $DB_NAME )
    set -e
    if [ -n "$DB" ]; then
      echo "ERROR :: Database $DB_NAME Already Exists, Aborting."
	  exit 1
    fi

    # =-=-=-=-=-=-=-
    # find postgres path & psql - modify config/irods.config accordingly
    EIRODSPOSTGRESDIR="$PGPATH/"
    echo "Detecting PostgreSQL Path: [$EIRODSPOSTGRESDIR]"
    sed -e "\,^\$DATABASE_HOME,s,^.*$,\$DATABASE_HOME = '$EIRODSPOSTGRESDIR';," $IRODS_HOME/config/irods.config > /tmp/irods.config.tmp
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
    # create the iCAT primary database
	echo "Creating Database: $DB_NAME as $DB_USER"
	su --shell=/bin/bash -c "createdb $DB_NAME" $DB_USER &> /dev/null
    # =-=-=-=-=-=-=-
    # create the default iCAT local zone database
        LOCAL_ZONE_DB_NAME=${DB_NAME}_9000
        echo "Creating Database: $LOCAL_ZONE_DB_NAME as $DB_USER"
        su --shell=/bin/bash -c "createdb $LOCAL_ZONE_DB_NAME" $DB_USER \
            &> /dev/null

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
    update-rc.d e-irods defaults
elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
    /sbin/chkconfig --add e-irods
elif [ "$DETECTEDOS" == "SuSE" ] ; then
    /sbin/chkconfig --add e-irods
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

ln -s    /usr/bin/genOSAuth               ${IRODS_HOME}/clients/icommands/bin/genOSAuth         
ln -s    /usr/bin/iadmin                  ${IRODS_HOME}/clients/icommands/bin/iadmin            
ln -s    /usr/bin/ibun                    ${IRODS_HOME}/clients/icommands/bin/ibun              
ln -s    /usr/bin/icd                     ${IRODS_HOME}/clients/icommands/bin/icd               
ln -s    /usr/bin/ichksum                 ${IRODS_HOME}/clients/icommands/bin/ichksum           
ln -s    /usr/bin/ichmod                  ${IRODS_HOME}/clients/icommands/bin/ichmod            
ln -s    /usr/bin/icp                     ${IRODS_HOME}/clients/icommands/bin/icp               
ln -s    /usr/bin/idbo                    ${IRODS_HOME}/clients/icommands/bin/idbo              
ln -s    /usr/bin/idbug                   ${IRODS_HOME}/clients/icommands/bin/idbug             
ln -s    /usr/bin/ienv                    ${IRODS_HOME}/clients/icommands/bin/ienv              
ln -s    /usr/bin/ierror                  ${IRODS_HOME}/clients/icommands/bin/ierror            
ln -s    /usr/bin/iexecmd                 ${IRODS_HOME}/clients/icommands/bin/iexecmd           
ln -s    /usr/bin/iexit                   ${IRODS_HOME}/clients/icommands/bin/iexit             
ln -s    /usr/bin/ifsck                   ${IRODS_HOME}/clients/icommands/bin/ifsck             
ln -s    /usr/bin/iget                    ${IRODS_HOME}/clients/icommands/bin/iget              
ln -s    /usr/bin/igetwild                ${IRODS_HOME}/clients/icommands/bin/igetwild          
ln -s    /usr/bin/igroupadmin             ${IRODS_HOME}/clients/icommands/bin/igroupadmin       
ln -s    /usr/bin/ihelp                   ${IRODS_HOME}/clients/icommands/bin/ihelp             
ln -s    /usr/bin/iinit                   ${IRODS_HOME}/clients/icommands/bin/iinit             
ln -s    /usr/bin/ilocate                 ${IRODS_HOME}/clients/icommands/bin/ilocate           
ln -s    /usr/bin/ils                     ${IRODS_HOME}/clients/icommands/bin/ils               
ln -s    /usr/bin/ilsresc                 ${IRODS_HOME}/clients/icommands/bin/ilsresc           
ln -s    /usr/bin/imcoll                  ${IRODS_HOME}/clients/icommands/bin/imcoll            
ln -s    /usr/bin/imeta                   ${IRODS_HOME}/clients/icommands/bin/imeta             
ln -s    /usr/bin/imiscsvrinfo            ${IRODS_HOME}/clients/icommands/bin/imiscsvrinfo      
ln -s    /usr/bin/imkdir                  ${IRODS_HOME}/clients/icommands/bin/imkdir            
ln -s    /usr/bin/imv                     ${IRODS_HOME}/clients/icommands/bin/imv               
ln -s    /usr/bin/ipasswd                 ${IRODS_HOME}/clients/icommands/bin/ipasswd           
ln -s    /usr/bin/iphybun                 ${IRODS_HOME}/clients/icommands/bin/iphybun           
ln -s    /usr/bin/iphymv                  ${IRODS_HOME}/clients/icommands/bin/iphymv            
ln -s    /usr/bin/ips                     ${IRODS_HOME}/clients/icommands/bin/ips               
ln -s    /usr/bin/iput                    ${IRODS_HOME}/clients/icommands/bin/iput              
ln -s    /usr/bin/ipwd                    ${IRODS_HOME}/clients/icommands/bin/ipwd              
ln -s    /usr/bin/iqdel                   ${IRODS_HOME}/clients/icommands/bin/iqdel             
ln -s    /usr/bin/iqmod                   ${IRODS_HOME}/clients/icommands/bin/iqmod             
ln -s    /usr/bin/iqstat                  ${IRODS_HOME}/clients/icommands/bin/iqstat            
ln -s    /usr/bin/iquest                  ${IRODS_HOME}/clients/icommands/bin/iquest            
ln -s    /usr/bin/iquota                  ${IRODS_HOME}/clients/icommands/bin/iquota            
ln -s    /usr/bin/ireg                    ${IRODS_HOME}/clients/icommands/bin/ireg              
ln -s    /usr/bin/irepl                   ${IRODS_HOME}/clients/icommands/bin/irepl             
ln -s    /usr/bin/irm                     ${IRODS_HOME}/clients/icommands/bin/irm               
ln -s    /usr/bin/irmtrash                ${IRODS_HOME}/clients/icommands/bin/irmtrash          
ln -s    /usr/bin/irsync                  ${IRODS_HOME}/clients/icommands/bin/irsync            
ln -s    /usr/bin/irule                   ${IRODS_HOME}/clients/icommands/bin/irule             
ln -s    /usr/bin/iscan                   ${IRODS_HOME}/clients/icommands/bin/iscan             
ln -s    /usr/bin/isysmeta                ${IRODS_HOME}/clients/icommands/bin/isysmeta          
ln -s    /usr/bin/itrim                   ${IRODS_HOME}/clients/icommands/bin/itrim             
ln -s    /usr/bin/iuserinfo               ${IRODS_HOME}/clients/icommands/bin/iuserinfo         
ln -s    /usr/bin/ixmsg                   ${IRODS_HOME}/clients/icommands/bin/ixmsg             









cd $PWD

# =-=-=-=-=-=-=-
# run setup script to configure an ICAT server
if [ "$SERVER_TYPE" == "icat" ] ; then
	cd $IRODS_HOME
	su --shell=/bin/bash -c "perl ./scripts/perl/eirods_setup.pl $DB_TYPE $DB_HOST $DB_PORT $DB_USER $DB_PASS" $OS_EIRODS_ACCT
fi


# =-=-=-=-=-=-=-
# remove setup 'rodsBoot' account - reduce potential attack surface
if [ "$SERVER_TYPE" == "icat" ] ; then
    su --shell=/bin/bash -c "/usr/bin/iadmin rmuser rodsBoot" $OS_EIRODS_ACCT
fi




# =-=-=-=-=-=-=-
if [ "$SERVER_TYPE" == "icat" ] ; then
  # tell user about their irodsenv
  cat $EIRODS_HOME_DIR/packaging/user_irodsenv.txt
  cat $EIRODS_HOME_DIR/.irods/.irodsEnv
elif [ "$SERVER_TYPE" == "resource" ] ; then
  # give user some guidance regarding resource configuration
  cat $EIRODS_HOME_DIR/packaging/user_resource.txt
fi

# =-=-=-=-=-=-=-
# really make sure everything is owned by the eirods service account
chown -R $OS_EIRODS_ACCT:$OS_EIRODS_ACCT $EIRODS_HOME_DIR

# =-=-=-=-=-=-=-
# exit with success
exit 0
























