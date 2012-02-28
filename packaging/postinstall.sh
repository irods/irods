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
cd $1
sudo -H -u $5 perl ./scripts/perl/eirods_setup.pl $2 $3 $4 $5 $6

# =-=-=-=-=-=-=-
# symlink irodsctl and the icommands
cd $1
ln -s ./irodsctl                             /usr/bin/irodsctl
ln -s ./clients/icommands/bin/genOSAuth      /usr/bin/genOSAuth
ln -s ./clients/icommands/bin/iadmin         /usr/bin/iadmin
ln -s ./clients/icommands/bin/ibun           /usr/bin/ibun
ln -s ./clients/icommands/bin/icd            /usr/bin/icd
ln -s ./clients/icommands/bin/ichksum        /usr/bin/ichksum
ln -s ./clients/icommands/bin/ichmod         /usr/bin/ichmod
ln -s ./clients/icommands/bin/icp            /usr/bin/icp
ln -s ./clients/icommands/bin/idbo           /usr/bin/idbo
ln -s ./clients/icommands/bin/idbug          /usr/bin/idbug
ln -s ./clients/icommands/bin/ienv           /usr/bin/ienv
ln -s ./clients/icommands/bin/ierror         /usr/bin/ierror
ln -s ./clients/icommands/bin/iexecmd        /usr/bin/iexecmd
ln -s ./clients/icommands/bin/iexit          /usr/bin/iexit
ln -s ./clients/icommands/bin/ifsck          /usr/bin/ifsck
ln -s ./clients/icommands/bin/iget           /usr/bin/iget
ln -s ./clients/icommands/bin/igetwild.sh    /usr/bin/igetwild.sh
ln -s ./clients/icommands/bin/ihelp          /usr/bin/ihelp
ln -s ./clients/icommands/bin/iinit          /usr/bin/iinit
ln -s ./clients/icommands/bin/ilocate        /usr/bin/ilocate
ln -s ./clients/icommands/bin/ils            /usr/bin/ils
ln -s ./clients/icommands/bin/ilsresc        /usr/bin/ilsresc
ln -s ./clients/icommands/bin/imcoll         /usr/bin/imcoll
ln -s ./clients/icommands/bin/imeta          /usr/bin/imeta
ln -s ./clients/icommands/bin/imiscsvrinfo   /usr/bin/imiscsvrinfo
ln -s ./clients/icommands/bin/imkdir         /usr/bin/imkdir
ln -s ./clients/icommands/bin/imv            /usr/bin/imv
ln -s ./clients/icommands/bin/ipasswd        /usr/bin/ipasswd
ln -s ./clients/icommands/bin/iphybun        /usr/bin/iphybun
ln -s ./clients/icommands/bin/iphymv         /usr/bin/iphymv
ln -s ./clients/icommands/bin/ips            /usr/bin/ips
ln -s ./clients/icommands/bin/iput           /usr/bin/iput
ln -s ./clients/icommands/bin/ipwd           /usr/bin/ipwd
ln -s ./clients/icommands/bin/iqdel          /usr/bin/iqdel
ln -s ./clients/icommands/bin/iqmod          /usr/bin/iqmod
ln -s ./clients/icommands/bin/iqstat         /usr/bin/iqstat
ln -s ./clients/icommands/bin/iquest         /usr/bin/iquest
ln -s ./clients/icommands/bin/iquota         /usr/bin/iquota
ln -s ./clients/icommands/bin/ireg           /usr/bin/ireg
ln -s ./clients/icommands/bin/irepl          /usr/bin/irepl
ln -s ./clients/icommands/bin/irm            /usr/bin/irm
ln -s ./clients/icommands/bin/irmtrash       /usr/bin/irmtrash
ln -s ./clients/icommands/bin/irsync         /usr/bin/irsync
ln -s ./clients/icommands/bin/irule          /usr/bin/irule
ln -s ./clients/icommands/bin/iscan          /usr/bin/iscan
ln -s ./clients/icommands/bin/isysmeta       /usr/bin/isysmeta
ln -s ./clients/icommands/bin/itrim          /usr/bin/itrim
ln -s ./clients/icommands/bin/iuserinfo      /usr/bin/iuserinfo
ln -s ./clients/icommands/bin/ixmsg          /usr/bin/ixmsg

# =-=-=-=-=-=-=-
# exit with success
exit 0

