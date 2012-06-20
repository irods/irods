#!/bin/sh

# variables
OUTDIR="coverageresults"
INFOFILE="coverage.info"

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
GENINFO=`which geninfo`
if [ "$?" -ne "0" ] ; then
    echo "ERROR :: geninfo is not in your path" 1>&2
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
geninfo -f -o $INFOFILE .
genhtml -o $OUTDIR $INFOFILE
rm $INFOFILE
