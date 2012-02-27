#!/bin/bash

# =-=-=-=-=-=-=-
# stop any running E-iRODS Processes
echo "*** Running Pre-Remove Script ***"
echo "Stopping iRODS :: $1/irodsctl stop"
sudo -u $2 $1/irodsctl stop

# =-=-=-=-=-=-=-
# determine if the database already exists
DB=$(sudo -u $3 psql --list  | grep $4 )
if [ -n "$DB" ]; then
  echo "Removing Database $4"
  sudo -u $3 dropdb $4
fi

# =-=-=-=-=-=-=-
# determine if the database role already exists
ROLE=$(sudo -u $3 psql postgres -tAc "SELECT 1 FROM pg_roles WHERE rolname='$2'")
if [ $ROLE ]; then
  echo "Removing Database Role $2"
  sudo -u $3 dropuser $2
fi

# =-=-=-=-=-=-=-
# determine if the service account already exists
USER=$( grep $2 /etc/passwd )
if [ -n "$USER" ]; then 
  echo "Removing Service Account $2"
  deluser $2
fi

rm /etc/init.d/e-irods
rm /etc/rc0.d/K15e-irods
rm /etc/rc2.d/S95e-irods
rm /etc/rc3.d/S95e-irods
rm /etc/rc4.d/S95e-irods
rm /etc/rc5.d/S95e-irods
rm /etc/rc6.d/K15e-irods

# =-=-=-=-=-=-=-
# exit with success
exit 0
