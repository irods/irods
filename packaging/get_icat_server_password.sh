#!/bin/bash -e

# get admin password, without showing on screen
read -s IRODS_ADMIN_PASSWORD
echo -n $IRODS_ADMIN_PASSWORD
