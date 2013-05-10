#!/bin/bash

###################################################################
#
#  A transitional new devtest that runs a combination of the
#  new python-based test suite as well as the legacy perl-based
#  iCAT test suite.
#
###################################################################

# detect self directory
SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )

# exit early if a command fails
set -e

# show commands as they are run
set -x

# run RENCI developed python-based devtest suite
# ( equivalent of original icommands and irules )
nosetests $SCRIPTPATH/../tests/pydevtest

# run DICE developed perl-based devtest suite
# ( icommands and irules have been turned off )
$SCRIPTPATH/../iRODS/irodsctl devtesty

# done
exit 0
