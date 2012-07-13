#!/bin/bash

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
GCOV=`which gcov`
if [ "$?" -ne "0" ] ; then
    echo "ERROR :: gcov is not in your path" 1>&2
    exit 1
fi
GENINFO=`which lcov`
if [ "$?" -ne "0" ] ; then
    echo "ERROR :: lcov is not in your path" 1>&2
    echo "      :: lcov source: http://downloads.sourceforge.net/ltp/lcov-1.9.tar.gz" 1>&2
    exit 1
fi
GENHTML=`which genhtml`
if [ "$?" -ne "0" ] ; then
    echo "ERROR :: genhtml is not in your path" 1>&2
    echo "      :: lcov source: http://downloads.sourceforge.net/ltp/lcov-1.9.tar.gz" 1>&2
    exit 1
fi

# generate the lcov results
for f in $(find . -name "*.gcno"); do foo=`pwd`;cd `dirname $f`; gcov `basename $f`; cd $foo; done
lcov -f -c --directory . -o $TESTINFOFILE                       # generate coverage file
lcov -a $BASEINFOFILE -a $TESTINFOFILE -o $TOTALINFOFILE        # combine base and test
lcov -r $TOTALINFOFILE "/usr/*" --directory . -o $TOTALINFOFILE # remove /usr/* lines from file
genhtml -o $OUTDIR $TOTALINFOFILE                               # generate html report

# clean up
rm $BASEINFOFILE
rm $TESTINFOFILE
rm $TOTALINFOFILE

