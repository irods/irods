#!/bin/bash

EIRODS_HOME_DIR=$1
OS_EIRODS_ACCT=$2
DB_TYPE=$3
DB_ADMIN_ROLE=$4
DB_NAME=$5
DB_USER=$6

IRODS_HOME=$EIRODS_HOME_DIR/iRODS

# =-=-=-=-=-=-=-
# stop any running E-iRODS Processes
echo "*** Running Pre-Remove Script ***"
echo "Stopping iRODS :: $IRODS_HOME/irodsctl stop"
su --shell=/bin/bash --session-command="$IRODS_HOME/irodsctl stop" $OS_EIRODS_ACCT

if [ "$SERVER_TYPE" == "icat" ] ; then

  if [ "$DB_TYPE" == "postgres" ] ; then
    # =-=-=-=-=-=-=-
    # determine if the database already exists
    PSQL=`$EIRODS_HOME_DIR/packaging/find_postgres.sh`
    DB=$( su --shell=/bin/bash --session-command="$PSQL --list | grep $DB_NAME" $DB_ADMIN_ROLE )
    if [ -n "$DB" ]; then
      echo "Removing Database $DB_NAME"
      su --shell=/bin/bash --session-command="dropdb $DB_NAME" $DB_ADMIN_ROLE
    fi

    # =-=-=-=-=-=-=-
    # determine if the database role already exists
    ROLE=$( su --shell=/bin/bash --session-command="$PSQL $DB_ADMIN_ROLE -tAc \"SELECT 1 FROM pg_roles WHERE rolname='$DB_USER'\"" $OS_EIRODS_ACCT )
    if [ $ROLE ]; then
      echo "Removing Database Role $DB_USER"
      su --shell=/bin/bash --session-command="dropuser $DB_USER" $DB_ADMIN_ROLE
    fi
  else
    # expand this for each type of database
    echo "TODO: remove non-postgres database"
    echo "TODO: remove non-postgres user"
  fi

fi

# =-=-=-=-=-=-=-
# determine if the service account already exists
USER=$( grep $OS_EIRODS_ACCT /etc/passwd )
if [ -n "$USER" ]; then 
  echo "Removing Service Account $OS_EIRODS_ACCT"
  cd /tmp
  userdel $OS_EIRODS_ACCT
fi

# =-=-=-=-=-=-=-
# remove runlevel symlinks
rm /etc/init.d/e-irods
rm /etc/rc0.d/K15e-irods
rm /etc/rc2.d/S95e-irods
rm /etc/rc3.d/S95e-irods
rm /etc/rc4.d/S95e-irods
rm /etc/rc5.d/S95e-irods
rm /etc/rc6.d/K15e-irods

# =-=-=-=-=-=-=-
# remove icommands symlinks
rm /usr/bin/chgCoreToCore1.ir
rm /usr/bin/chgCoreToCore2.ir
rm /usr/bin/chgCoreToOrig.ir
rm /usr/bin/delUnusedAVUs.ir
rm /usr/bin/genOSAuth
rm /usr/bin/iadmin
rm /usr/bin/ibun
rm /usr/bin/icd
rm /usr/bin/ichksum
rm /usr/bin/ichmod
rm /usr/bin/icp
rm /usr/bin/idbo
rm /usr/bin/idbug
rm /usr/bin/ienv
rm /usr/bin/ierror
rm /usr/bin/iexecmd
rm /usr/bin/iexit
rm /usr/bin/ifsck
rm /usr/bin/iget
rm /usr/bin/igetwild.sh
rm /usr/bin/ihelp
rm /usr/bin/iinit
rm /usr/bin/ilocate
rm /usr/bin/ils
rm /usr/bin/ilsresc
rm /usr/bin/imcoll
rm /usr/bin/imeta
rm /usr/bin/imiscsvrinfo
rm /usr/bin/imkdir
rm /usr/bin/imv
rm /usr/bin/ipasswd
rm /usr/bin/iphybun
rm /usr/bin/iphymv
rm /usr/bin/ips
rm /usr/bin/iput
rm /usr/bin/ipwd
rm /usr/bin/iqdel
rm /usr/bin/iqmod
rm /usr/bin/iqstat
rm /usr/bin/iquest
rm /usr/bin/iquota
rm /usr/bin/ireg
rm /usr/bin/irepl
rm /usr/bin/irm
rm /usr/bin/irmtrash
rm /usr/bin/irsync
rm /usr/bin/irule
rm /usr/bin/iscan
rm /usr/bin/isysmeta
rm /usr/bin/itrim
rm /usr/bin/iuserinfo
rm /usr/bin/ixmsg
rm /usr/bin/runQuota.ir
rm /usr/bin/runQuota.r
rm /usr/bin/showCore.ir

# =-=-=-=-=-=-=-
# exit with success
exit 0
