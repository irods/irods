#!/bin/bash

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
# clean up any stray iRODS files in /tmp which will cause problems
if [ -f /tmp/irodsServer.* ]; then
  rm /tmp/irodsServer.*
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
    # determine if the database already exists
    DB=$( su --shell=/bin/bash -c "$PSQL --list" $DB_ADMIN_ROLE  | grep $DB_NAME )
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

	PSQL="$PGPATH/psql"

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
# symlink init.d script to rcX.d
PWD=`pwd`
cd /etc/rc0.d
file="K15e-irods"
if [ -e ./$file ]; then
    res=`ls -l ./$file | cut -d' ' -f11`
    if [ "$res" != "../init.d/e-irods" ]; then
		rm $file
        ln -s ../init.d/e-irods ./$file
	fi
else
    ln -s ../init.d/e-irods ./$file
fi

cd /etc/rc2.d
file="S95e-irods"
if [ -e ./$file ]; then
    res=`ls -l ./$file | cut -d' ' -f11`
    if [ "$res" != "../init.d/e-irods" ]; then
		rm $file
        ln -s ../init.d/e-irods ./$file
	fi
else
    ln -s ../init.d/e-irods ./$file
fi

cd /etc/rc3.d
file="S95e-irods"
if [ -e ./$file ]; then
    res=`ls -l ./$file | cut -d' ' -f11`
    if [ "$res" != "../init.d/e-irods" ]; then
		rm $file
        ln -s ../init.d/e-irods ./$file
	fi
else
    ln -s ../init.d/e-irods ./$file
fi

cd /etc/rc4.d
file="S95e-irods"
if [ -e ./$file ]; then
    res=`ls -l ./$file | cut -d' ' -f11`
    if [ "$res" != "../init.d/e-irods" ]; then
		rm $file
        ln -s ../init.d/e-irods ./$file
	fi
else
    ln -s ../init.d/e-irods ./$file
fi

cd /etc/rc5.d
file="S95e-irods"
if [ -e ./$file ]; then
    res=`ls -l ./$file | cut -d' ' -f11`
    if [ "$res" != "../init.d/e-irods" ]; then
		rm $file
        ln -s ../init.d/e-irods ./$file
	fi
else
    ln -s ../init.d/e-irods ./$file
fi

cd /etc/rc6.d
file="K15e-irods"
if [ -e ./$file ]; then
    res=`ls -l ./$file | cut -d' ' -f11`
    if [ "$res" != "../init.d/e-irods" ]; then
		rm $file
        ln -s ../init.d/e-irods ./$file
	fi
else
    ln -s ../init.d/e-irods ./$file
fi




cd $PWD

# =-=-=-=-=-=-=-
# run setup script to configure an ICAT server
if [ "$SERVER_TYPE" == "icat" ] ; then
	cd $IRODS_HOME
	su --shell=/bin/bash -c "perl ./scripts/perl/eirods_setup.pl $DB_TYPE $DB_HOST $DB_PORT $DB_USER $DB_PASS" $OS_EIRODS_ACCT
fi

# =-=-=-=-=-=-=-
# symlink the icommands
cd $IRODS_HOME

for file in `ls ${IRODS_HOME}/clients/icommands/bin/`
do
	if [ -e /usr/bin/$file ]; then
        res=`ls -l /usr/bin | cut -d' ' -f11`
		if [ "$res" == "${IRODS_HOME}/clients/icommands/bin/$file" ]; then
			continue
		else
			rm /usr/bin/$file
		fi
	fi

	ln -s ${IRODS_HOME}/clients/icommands/bin/$file /usr/bin/$file

done



#ln -s ${IRODS_HOME}/clients/icommands/bin/chgCoreToCore1.ir    /usr/bin/chgCoreToCore1.ir
#ln -s ${IRODS_HOME}/clients/icommands/bin/chgCoreToCore2.ir    /usr/bin/chgCoreToCore2.ir
#ln -s ${IRODS_HOME}/clients/icommands/bin/chgCoreToOrig.ir     /usr/bin/chgCoreToOrig.ir
#ln -s ${IRODS_HOME}/clients/icommands/bin/delUnusedAVUs.ir     /usr/bin/delUnusedAVUs.ir
#ln -s ${IRODS_HOME}/clients/icommands/bin/genOSAuth            /usr/bin/genOSAuth
#ln -s ${IRODS_HOME}/clients/icommands/bin/iadmin               /usr/bin/iadmin
#ln -s ${IRODS_HOME}/clients/icommands/bin/ibun                 /usr/bin/ibun
#ln -s ${IRODS_HOME}/clients/icommands/bin/icd                  /usr/bin/icd
#ln -s ${IRODS_HOME}/clients/icommands/bin/ichksum              /usr/bin/ichksum
#ln -s ${IRODS_HOME}/clients/icommands/bin/ichmod               /usr/bin/ichmod
#ln -s ${IRODS_HOME}/clients/icommands/bin/icp                  /usr/bin/icp
#ln -s ${IRODS_HOME}/clients/icommands/bin/idbo                 /usr/bin/idbo
#ln -s ${IRODS_HOME}/clients/icommands/bin/idbug                /usr/bin/idbug
#ln -s ${IRODS_HOME}/clients/icommands/bin/ienv                 /usr/bin/ienv
#ln -s ${IRODS_HOME}/clients/icommands/bin/ierror               /usr/bin/ierror
#ln -s ${IRODS_HOME}/clients/icommands/bin/iexecmd              /usr/bin/iexecmd
#ln -s ${IRODS_HOME}/clients/icommands/bin/iexit                /usr/bin/iexit
#ln -s ${IRODS_HOME}/clients/icommands/bin/ifsck                /usr/bin/ifsck
#ln -s ${IRODS_HOME}/clients/icommands/bin/iget                 /usr/bin/iget
#ln -s ${IRODS_HOME}/clients/icommands/bin/igetwild.sh          /usr/bin/igetwild.sh
#ln -s ${IRODS_HOME}/clients/icommands/bin/ihelp                /usr/bin/ihelp
#ln -s ${IRODS_HOME}/clients/icommands/bin/iinit                /usr/bin/iinit
#ln -s ${IRODS_HOME}/clients/icommands/bin/ilocate              /usr/bin/ilocate
#ln -s ${IRODS_HOME}/clients/icommands/bin/ils                  /usr/bin/ils
#ln -s ${IRODS_HOME}/clients/icommands/bin/ilsresc              /usr/bin/ilsresc
#ln -s ${IRODS_HOME}/clients/icommands/bin/imcoll               /usr/bin/imcoll
#ln -s ${IRODS_HOME}/clients/icommands/bin/imeta                /usr/bin/imeta
#ln -s ${IRODS_HOME}/clients/icommands/bin/imiscsvrinfo         /usr/bin/imiscsvrinfo
#ln -s ${IRODS_HOME}/clients/icommands/bin/imkdir               /usr/bin/imkdir
#ln -s ${IRODS_HOME}/clients/icommands/bin/imv                  /usr/bin/imv
#ln -s ${IRODS_HOME}/clients/icommands/bin/ipasswd              /usr/bin/ipasswd
#ln -s ${IRODS_HOME}/clients/icommands/bin/iphybun              /usr/bin/iphybun
#ln -s ${IRODS_HOME}/clients/icommands/bin/iphymv               /usr/bin/iphymv
#ln -s ${IRODS_HOME}/clients/icommands/bin/ips                  /usr/bin/ips
#ln -s ${IRODS_HOME}/clients/icommands/bin/iput                 /usr/bin/iput
#ln -s ${IRODS_HOME}/clients/icommands/bin/ipwd                 /usr/bin/ipwd
#ln -s ${IRODS_HOME}/clients/icommands/bin/iqdel                /usr/bin/iqdel
#ln -s ${IRODS_HOME}/clients/icommands/bin/iqmod                /usr/bin/iqmod
#ln -s ${IRODS_HOME}/clients/icommands/bin/iqstat               /usr/bin/iqstat
#ln -s ${IRODS_HOME}/clients/icommands/bin/iquest               /usr/bin/iquest
#ln -s ${IRODS_HOME}/clients/icommands/bin/iquota               /usr/bin/iquota
#ln -s ${IRODS_HOME}/clients/icommands/bin/ireg                 /usr/bin/ireg
#ln -s ${IRODS_HOME}/clients/icommands/bin/irepl                /usr/bin/irepl
#ln -s ${IRODS_HOME}/clients/icommands/bin/irm                  /usr/bin/irm
#ln -s ${IRODS_HOME}/clients/icommands/bin/irmtrash             /usr/bin/irmtrash
#ln -s ${IRODS_HOME}/clients/icommands/bin/irsync               /usr/bin/irsync
#ln -s ${IRODS_HOME}/clients/icommands/bin/irule                /usr/bin/irule
#ln -s ${IRODS_HOME}/clients/icommands/bin/iscan                /usr/bin/iscan
#ln -s ${IRODS_HOME}/clients/icommands/bin/isysmeta             /usr/bin/isysmeta
#ln -s ${IRODS_HOME}/clients/icommands/bin/itrim                /usr/bin/itrim
#ln -s ${IRODS_HOME}/clients/icommands/bin/iuserinfo            /usr/bin/iuserinfo
#ln -s ${IRODS_HOME}/clients/icommands/bin/ixmsg                /usr/bin/ixmsg
#ln -s ${IRODS_HOME}/clients/icommands/bin/runQuota.ir          /usr/bin/runQuota.ir
#ln -s ${IRODS_HOME}/clients/icommands/bin/runQuota.r           /usr/bin/runQuota.r
#ln -s ${IRODS_HOME}/clients/icommands/bin/showCore.ir          /usr/bin/showCore.ir

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
























