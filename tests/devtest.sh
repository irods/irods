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
        wget -nc ftp://ftp.renci.org/pub/irods/external/$UNITTEST2VERSION.tar.gz
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
if [ "$1" == "ci" ] ; then
    IRODSDEVTESTCI="true"
fi

# check for topological testing parameter
if [ "$1" == "topo" ] ; then
    IRODSDEVTESTTOPO="true"
fi

# if running as a human
if [ "$IRODSDEVTESTCI" != "true" ] ; then
    # human user, allows keyboard interrupt to clean up
    OPTS="$OPTS -c "
fi

# pass any other parameters to the test framework
if [ "$IRODSDEVTESTCI" == "true" -o "$IRODSDEVTESTTOPO" == "true" ] ; then
    # trim the ci parameter
    shift
    PYTESTS=$@
else
    PYTESTS=$@
fi

#########################
# run tests
#########################

# detect iRODS root directory
IRODSROOT=$( dirname $( cd $(dirname $0) ; pwd -P ) )

# exit early if a command fails
set -e

# show commands as they are run
set -x

# restart the server, to exercise that code
cd $IRODSROOT
$IRODSROOT/iRODS/irodsctl restart

# run RENCI developed python-based devtest suite (or just specified tests)
# ( equivalent of original icommands and irules )
cd $IRODSROOT/tests/pydevtest
if [ "$PYTHONVERSION" \< "2.7" ] ; then
    cp -r ../$UNITTEST2VERSION/unittest2 .
    cp ../$UNITTEST2VERSION/unit2 .
fi
# run the suite
if [ "$PYTESTS" != "" ] ; then
    $PYTHONCMD $OPTS $PYTESTS
else
    $PYTHONCMD $OPTS test_resource_types
    $PYTHONCMD $OPTS iadmin_suite
    $PYTHONCMD $OPTS catalog_suite
    $PYTHONCMD $OPTS test_workflow_suite
    $PYTHONCMD $OPTS test_mso_suite
    $PYTHONCMD $OPTS test_resource_tree
    $PYTHONCMD $OPTS test_xmsg
    $PYTHONCMD $OPTS test_load_balanced_suite
    nosetests -v test_allrules.py
    if [ ! "$IRODSDEVTESTTOPO" == "true" ] ; then
        # run DICE developed perl-based devtest suite
        cd $IRODSROOT
        $IRODSROOT/iRODS/irodsctl devtesty
    fi
fi

# run authentication tests
if [ "$IRODSDEVTESTCI" == "true" ] ; then
    cd $IRODSROOT/tests/pydevtest
    $PYTHONCMD $OPTS auth_suite.Test_Auth_Suite
fi

# run OSAuth test by itself
if [ "$IRODSDEVTESTCI" == "true" ] ; then
    cd $IRODSROOT/tests/pydevtest
    set +e
    passwd <<EOF
temporarypasswordforci
temporarypasswordforci
EOF
    PASSWDRESULT=`echo $?`
    if [ "$PASSWDRESULT" != 0 ] ; then
            # known suse11 behavior
            # needs an empty line for 'old password' prompt
            passwd <<EOF

temporarypasswordforci
temporarypasswordforci
EOF
    fi
    PASSWDRESULT=`echo $?`
    if [ "$PASSWDRESULT" != 0 ] ; then
        exit $PASSWDRESULT
    fi
    set -e
    cd $IRODSROOT/tests/pydevtest
    $PYTHONCMD $OPTS auth_suite.Test_OSAuth_Only
    ################################################
    # note:
    #   this test is run last to minimize the
    #   window of the following...
    # side effect:
    #   unix irods user now has a set password
    # to remove, run:
    #   sudo passwd -d irods
    ################################################
fi

# clean up /tmp
set +x
ls /tmp/psqlodbc* | xargs rm -f
set -x

# done
exit 0
