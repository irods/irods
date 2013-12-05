#!/bin/bash

###################################################################
#
#  A script to kill all existing processes owned by the unix user
#  "irods".  This will assure that the continuous integration
#  server can assume that all zombie processes are handled before
#  starting any next step.
#
#  Expected use:
#
#  $ ./iRODS/irodsctl stop
#  $ ./tests/zombiereaper.sh
#  $ ./iRODS/irodsctl start
#
###################################################################

pgrep -u irods irods | xargs kill -9

##############
# Zombies Dead
##############
exit 0
