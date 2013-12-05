#!/bin/bash

#######################################################################
#
# This script should be run after upgrading from an E-iRODS 3.0 RPM.
#
# It takes one parameter:
#    newfile.rpm - the path to the new RPM
#
# Requires being run as root.
#
#######################################################################

set -e

if [ $# -ne 1 ] ; then
   echo "Usage: $0 newfile.rpm"
   exit 1
fi

# repave all the files
rpm -U --replacepkgs --replacefiles $1 > /dev/null

# cleanly recover from the upgrade mechanism from 3.0
if [ -d /var/lib/eirods_new ] ; then
    rm -rf /var/lib/eirods
    rm -rf /var/lib/irods
    mv /var/lib/eirods_new /var/lib/irods
fi

# restart the server
su - irods -c "cd iRODS; ./irodsctl restart"

# report to the admin
echo ""
echo "#########################################################"
echo "#"
echo "#  iRODS RPM installation has been upgraded from 3.0"
echo "#"
echo "#########################################################"
echo ""

# done
exit 0
