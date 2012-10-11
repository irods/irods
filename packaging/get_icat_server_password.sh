#!/bin/bash -e

# get admin password, without showing on screen
stty -echo
read IRODS_ADMIN_PASSWORD
stty echo
echo -n $IRODS_ADMIN_PASSWORD

