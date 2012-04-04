#!/bin/bash

IRODS_HOME=$1
SVC_ACCT=$2
SERVERTYPE=$3
DATABASE_ROLE=$4
DATABASE=$5

# =-=-=-=-=-=-=-
# stop any running E-iRODS Processes
echo "*** Running Pre-Remove Script ***"
echo "Stopping iRODS :: $IRODS_HOME/irodsctl stop"
cd $IRODS_HOME
sudo -u $SVC_ACCT $IRODS_HOME/irodsctl stop

if [ "$SERVERTYPE" == "icat" ] ; then
  # =-=-=-=-=-=-=-
  # determine if the database already exists
  DB=$(sudo -u $SVC_ACCT psql --list  | grep $DATABASE )
  if [ -n "$DB" ]; then
    echo "Removing Database $DATABASE"
    sudo -u $DATABASE_ROLE dropdb $DATABASE
  fi

  # =-=-=-=-=-=-=-
  # determine if the database role already exists
  ROLE=$(sudo -u $SVC_ACCT psql postgres -tAc "SELECT 1 FROM pg_roles WHERE rolname='$DATABASE_ROLE'")
  if [ $ROLE ]; then
    echo "Removing Database Role $DATABASE_ROLE"
    sudo -u $SVC_ACCT dropuser $DATABASE_ROLE
  fi
fi

# =-=-=-=-=-=-=-
# determine if the service account already exists
USER=$( grep $SVC_ACCT /etc/passwd )
if [ -n "$USER" ]; then 
  echo "Removing Service Account $SVC_ACCT"
  cd /tmp
  userdel $SVC_ACCT
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
