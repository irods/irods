#!/bin/sh

# =-=-=-=-=-=-=-
# clean up any stray irods files in /tmp which will cause problems
if [ -f /tmp/irodsServer.* ]; then
  rm /tmp/irodsServer.*
fi

# =-=-=-=-=-=-=-
# determine if the service account already exists
USER=$( grep $5 /etc/passwd )
if [ -n "$USER" ]; then 
  echo "WARNING :: Service Account $5 Already Exists."
else
  # =-=-=-=-=-=-=-
  # create the service account
  echo "Creating Service Account: $5 At $9"
  useradd -m -d $9 $5
  chown $5:$5 $9
fi

# =-=-=-=-=-=-=-
# determine if the database role already exists
ROLE=$(sudo -u $7 psql postgres -tAc "SELECT 1 FROM pg_roles WHERE rolname='$5'")
if [ $ROLE ]; then
  echo "WARNING :: Role $5 Already Exists in Database."
else
  # =-=-=-=-=-=-=-
  # create the database role
  echo "Creating Database Role: $5 As $7"
  sudo -u $7 createuser -s $5
fi

# =-=-=-=-=-=-=-
# determine if the database already exists
DB=$(sudo -u $7 psql --list  | grep $8 )
if [ -n "$DB" ]; then
  echo "WARNING :: Database $8 Already Exists"
fi

# =-=-=-=-=-=-=-
# set permisions on the installed files
chown -R $5:$5 $1

# =-=-=-=-=-=-=-
# touch odbc file so it exists for the install script to update 
touch $9/.odbc.ini
chown $5:$5 $9/.odbc.ini

# =-=-=-=-=-=-=-
# symlink init.d script to rc0.d
ln -s /etc/init.d/e-irods /etc/rc0.d/K15e-irods

# =-=-=-=-=-=-=-
# run setup script to configure database, users, default resource, etc.
cd $1
sudo -H -u $5 perl ./scripts/perl/eirods_setup.pl $2 $3 $4 $5 $6

# =-=-=-=-=-=-=-
# exit with success
exit 0

