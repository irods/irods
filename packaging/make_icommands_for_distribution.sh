#!/bin/bash

# this script bundles the icommands and required client-side plugins
# for distribution to users who do not have the necessary root privileges
# to install the OS level binary packages

# the script will create a tgz file ( icommands.tgz ) which may be extracted in
# the user's home directory.  the irods_plugins_home variable in the 
# .irods/irods_environment.json file must be set to the fully qualified path
# of the icommands/plugins/ directory:
#
# {
#     "irods_plugins_home" : "/home/joeuser/bin/icommands/plugins/"
# 
# }
#
# once this file is created, iinit may be run to finish setting up the
# irods environment.

SCRIPTPATH=$( cd $(dirname $0)/.. ; pwd -P )

mkdir -p $SCRIPTPATH/icommands
mkdir -p $SCRIPTPATH/icommands/plugins
mkdir -p $SCRIPTPATH/icommands/plugins/auth
mkdir -p $SCRIPTPATH/icommands/plugins/network

cp $SCRIPTPATH/iRODS/clients/icommands/bin/* $SCRIPTPATH/icommands

cp $SCRIPTPATH/plugins/auth/*.so $SCRIPTPATH/icommands/plugins/auth
cp $SCRIPTPATH/plugins/network/*.so $SCRIPTPATH/icommands/plugins/network

tar zcf icommands.tgz icommands

rm -rf $SCRIPTPATH/icommands

