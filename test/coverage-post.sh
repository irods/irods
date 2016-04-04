#!/bin/bash

# exit on error
set -e

# variables
OUTDIR="coverageresults"
BASEINFOFILE="coverage-base.info"
TESTINFOFILE="coverage-test.info"
TOTALINFOFILE="coverage-total.info"

# get into the correct directory
DETECTEDDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DETECTEDDIR/../

# error checking
if [ "$1" != "" ] ; then
    OUTDIR=$1
fi
set +e
GCOV=`which gcov`
set -e
if [ "$GCOV" = "" ] ; then
    echo "ERROR :: gcov is not in your path" 1>&2
    exit 1
fi
set +e
GENINFO=`which lcov`
set -e
if [ "$GENINFO" = "" ] ; then
    echo "ERROR :: lcov is not in your path" 1>&2
    echo "      :: lcov source: http://downloads.sourceforge.net/ltp/lcov-1.10.tar.gz" 1>&2
    exit 1
fi
set +e
GENHTML=`which genhtml`
set -e
if [ "$GENHTML" = "" ] ; then
    echo "ERROR :: genhtml is not in your path" 1>&2
    echo "      :: lcov source: http://downloads.sourceforge.net/ltp/lcov-1.10.tar.gz" 1>&2
    exit 1
fi

# generate the lcov results
for f in $(find . -name "*.gcno"); do foo=`pwd`;cd `dirname $f`; gcov `basename $f`; cd $foo; done
lcov -f -c --directory . -o $TESTINFOFILE                           # generate coverage file
lcov -a $BASEINFOFILE -a $TESTINFOFILE -o $TOTALINFOFILE            # combine base and test
lcov -r $TOTALINFOFILE "/usr/*" --directory . -o $TOTALINFOFILE     # remove /usr/* lines from file
lcov -r $TOTALINFOFILE "external/*" --directory . -o $TOTALINFOFILE # remove external/* lines from file
genhtml -o $OUTDIR $TOTALINFOFILE                                   # generate html report

# clean up
#rm $BASEINFOFILE
rm $TESTINFOFILE
rm $TOTALINFOFILE

