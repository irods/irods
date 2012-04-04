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
DB_PASS=$10

IRODS_HOME=$EIRODS_HOME_DIR/iRODS

# =-=-=-=-=-=-=-
# clean up any stray iRODS files in /tmp which will cause problems
if [ -f /tmp/irodsServer.* ]; then
  rm /tmp/irodsServer.*
fi

if [ "$SERVER_TYPE" == "icat" ] ; then

  if [ "$DB_TYPE" == "postgres" ] ; then
    # =-=-=-=-=-=-=-
    # detect database path and update installed irods.config accordingly
    PSQL=`$EIRODS_HOME_DIR/packaging/find_postgres.sh`
    EIRODSPOSTGRESPATH=`$PSQL | sed -e s,\/[^\/]*$,, -e s,\/[^\/]*$,,`
    EIRODSPOSTGRESPATH="$EIRODSPOSTGRESPATH/"
    echo "Detecting PostgreSQL Path: [$EIRODSPOSTGRESPATH]"
    sed -e ,^\$DATABASE_HOME,s,^.*$,"\$DATABASE_HOME = '$EIRODSPOSTGRESPATH';", $IRODS_HOME/config/irods.config > /tmp/irods.config.tmp
    mv /tmp/irods.config.tmp $IRODS_HOME/config/irods.config

    # =-=-=-=-=-=-=-
    # determine if the database role already exists
    ROLE=$( su --shell=/bin/bash --session-command="$PSQL $DB_ADMIN_ROLE -tAc \"SELECT 1 FROM pg_roles WHERE rolname='$DB_USER'\"" $DB_ADMIN_ROLE )
    if [ $ROLE ]; then
      echo "WARNING :: Role $DB_USER Already Exists in Database."
    else
      # =-=-=-=-=-=-=-
      # create the database role
      echo "Creating Database Role: $DB_USER as $DB_ADMIN_ROLE"
      su --shell=/bin/sh --session-command="createuser -s $DB_USER" $DB_ADMIN_ROLE
    fi

    # =-=-=-=-=-=-=-
    # determine if the database already exists
    DB=$( su --shell=/bin/bash --session-command="$PSQL --list" $DB_ADMIN_ROLE  | grep $DB_NAME )
    if [ -n "$DB" ]; then
      echo "WARNING :: Database $DB_NAME Already Exists"
    fi

  else

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
ln -s ../init.d/e-irods ./K15e-irods

cd /etc/rc2.d
ln -s ../init.d/e-irods ./S95e-irods

cd /etc/rc3.d
ln -s ../init.d/e-irods ./S95e-irods

cd /etc/rc4.d
ln -s ../init.d/e-irods ./S95e-irods

cd /etc/rc5.d
ln -s ../init.d/e-irods ./S95e-irods

cd /etc/rc6.d
ln -s ../init.d/e-irods ./K15e-irods

cd $PWD

# =-=-=-=-=-=-=-
# run setup script to configure database, users, default resource, etc.
cd $IRODS_HOME
su --shell=/bin/bash --session-command="perl ./scripts/perl/eirods_setup.pl $DB_TYPE $DB_HOST $DB_PORT $DB_USER $DB_PASS" $OS_EIRODS_ACCT

# =-=-=-=-=-=-=-
# symlink the icommands
cd $IRODS_HOME
ln -s ${1}/clients/icommands/bin/chgCoreToCore1.ir    /usr/bin/chgCoreToCore1.ir
ln -s ${1}/clients/icommands/bin/chgCoreToCore2.ir    /usr/bin/chgCoreToCore2.ir
ln -s ${1}/clients/icommands/bin/chgCoreToOrig.ir     /usr/bin/chgCoreToOrig.ir
ln -s ${1}/clients/icommands/bin/delUnusedAVUs.ir     /usr/bin/delUnusedAVUs.ir
ln -s ${1}/clients/icommands/bin/genOSAuth            /usr/bin/genOSAuth
ln -s ${1}/clients/icommands/bin/iadmin               /usr/bin/iadmin
ln -s ${1}/clients/icommands/bin/ibun                 /usr/bin/ibun
ln -s ${1}/clients/icommands/bin/icd                  /usr/bin/icd
ln -s ${1}/clients/icommands/bin/ichksum              /usr/bin/ichksum
ln -s ${1}/clients/icommands/bin/ichmod               /usr/bin/ichmod
ln -s ${1}/clients/icommands/bin/icp                  /usr/bin/icp
ln -s ${1}/clients/icommands/bin/idbo                 /usr/bin/idbo
ln -s ${1}/clients/icommands/bin/idbug                /usr/bin/idbug
ln -s ${1}/clients/icommands/bin/ienv                 /usr/bin/ienv
ln -s ${1}/clients/icommands/bin/ierror               /usr/bin/ierror
ln -s ${1}/clients/icommands/bin/iexecmd              /usr/bin/iexecmd
ln -s ${1}/clients/icommands/bin/iexit                /usr/bin/iexit
ln -s ${1}/clients/icommands/bin/ifsck                /usr/bin/ifsck
ln -s ${1}/clients/icommands/bin/iget                 /usr/bin/iget
ln -s ${1}/clients/icommands/bin/igetwild.sh          /usr/bin/igetwild.sh
ln -s ${1}/clients/icommands/bin/ihelp                /usr/bin/ihelp
ln -s ${1}/clients/icommands/bin/iinit                /usr/bin/iinit
ln -s ${1}/clients/icommands/bin/ilocate              /usr/bin/ilocate
ln -s ${1}/clients/icommands/bin/ils                  /usr/bin/ils
ln -s ${1}/clients/icommands/bin/ilsresc              /usr/bin/ilsresc
ln -s ${1}/clients/icommands/bin/imcoll               /usr/bin/imcoll
ln -s ${1}/clients/icommands/bin/imeta                /usr/bin/imeta
ln -s ${1}/clients/icommands/bin/imiscsvrinfo         /usr/bin/imiscsvrinfo
ln -s ${1}/clients/icommands/bin/imkdir               /usr/bin/imkdir
ln -s ${1}/clients/icommands/bin/imv                  /usr/bin/imv
ln -s ${1}/clients/icommands/bin/ipasswd              /usr/bin/ipasswd
ln -s ${1}/clients/icommands/bin/iphybun              /usr/bin/iphybun
ln -s ${1}/clients/icommands/bin/iphymv               /usr/bin/iphymv
ln -s ${1}/clients/icommands/bin/ips                  /usr/bin/ips
ln -s ${1}/clients/icommands/bin/iput                 /usr/bin/iput
ln -s ${1}/clients/icommands/bin/ipwd                 /usr/bin/ipwd
ln -s ${1}/clients/icommands/bin/iqdel                /usr/bin/iqdel
ln -s ${1}/clients/icommands/bin/iqmod                /usr/bin/iqmod
ln -s ${1}/clients/icommands/bin/iqstat               /usr/bin/iqstat
ln -s ${1}/clients/icommands/bin/iquest               /usr/bin/iquest
ln -s ${1}/clients/icommands/bin/iquota               /usr/bin/iquota
ln -s ${1}/clients/icommands/bin/ireg                 /usr/bin/ireg
ln -s ${1}/clients/icommands/bin/irepl                /usr/bin/irepl
ln -s ${1}/clients/icommands/bin/irm                  /usr/bin/irm
ln -s ${1}/clients/icommands/bin/irmtrash             /usr/bin/irmtrash
ln -s ${1}/clients/icommands/bin/irsync               /usr/bin/irsync
ln -s ${1}/clients/icommands/bin/irule                /usr/bin/irule
ln -s ${1}/clients/icommands/bin/iscan                /usr/bin/iscan
ln -s ${1}/clients/icommands/bin/isysmeta             /usr/bin/isysmeta
ln -s ${1}/clients/icommands/bin/itrim                /usr/bin/itrim
ln -s ${1}/clients/icommands/bin/iuserinfo            /usr/bin/iuserinfo
ln -s ${1}/clients/icommands/bin/ixmsg                /usr/bin/ixmsg
ln -s ${1}/clients/icommands/bin/runQuota.ir          /usr/bin/runQuota.ir
ln -s ${1}/clients/icommands/bin/runQuota.r           /usr/bin/runQuota.r
ln -s ${1}/clients/icommands/bin/showCore.ir          /usr/bin/showCore.ir

if [ "$SERVER_TYPE" == "resource" ] ; then
  # =-=-=-=-=-=-=-
  # prompt for resource server configuration information
  $EIRODS_HOME_DIR/packaging/setup_resource.sh
fi

# =-=-=-=-=-=-=-
# give user some guidance regarding .irodsEnv
cat $EIRODS_HOME_DIR/packaging/user_help.txt | sed -e s/localhost/`hostname`/

# =-=-=-=-=-=-=-
# exit with success
exit 0
