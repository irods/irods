#!/bin/bash

###################################################################
#
#  A transitional new devtest that runs a combination of the
#  new python-based test suite as well as the legacy perl-based
#  iCAT test suite.
#
###################################################################

#########################
# preflight
#########################

# check python version
PYTHONVERSION=$( python --version 2>&1 | awk '{print $2}' )
if [ "$PYTHONVERSION" \< "2.7" ] ; then
    # get unittest2 package
    UNITTEST2VERSION="unittest2-0.5.1"
    if [ ! -e $UNITTEST2VERSION.tar.gz ] ; then
        wget -nc ftp://ftp.renci.org/pub/eirods/external/$UNITTEST2VERSION.tar.gz
    fi
    if [ ! -e $UNITTEST2VERSION.tar ] ; then
        gunzip $UNITTEST2VERSION.tar.gz
    fi
    tar xf $UNITTEST2VERSION.tar
    cd $UNITTEST2VERSION
    python setup.py build
    cd ..
    PYTHONCMD="./unit2"
else
    # python 2.7+
    PYTHONCMD="python -m unittest"
fi

# set command line options for test runner
OPTS=" -v -b -f "

# check for continuous integration parameter
if [ "$1" != "ci" ] ; then
    # human user, allows keyboard interrupt to clean up
    OPTS="$OPTS -c "
fi


#########################
# run tests
#########################

# detect E-iRODS root directory
EIRODSROOT=$( dirname $( cd $(dirname $0) ; pwd -P ) )

# exit early if a command fails
set -e

# show commands as they are run
set -x

# restart the server, to exercise that code
cd $EIRODSROOT
$EIRODSROOT/iRODS/irodsctl restart

# run RENCI developed python-based devtest suite
# ( equivalent of original icommands and irules )
cd $EIRODSROOT/tests/pydevtest
if [ "$PYTHONVERSION" \< "2.7" ] ; then
    cp -r ../$UNITTEST2VERSION/unittest2 .
    cp ../$UNITTEST2VERSION/unit2 .
fi
$PYTHONCMD $OPTS test_eirods_resource_types iadmin_suite catalog_suite
nosetests -v test_allrules.py

# run DICE developed perl-based devtest suite
cd $EIRODSROOT
$EIRODSROOT/iRODS/irodsctl devtesty

# clean up /tmp
set +x
ls /tmp/psqlodbc* | xargs rm -f
set -x

# done
exit 0
