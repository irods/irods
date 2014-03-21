#!/bin/bash

# exit on error
set -e

# variables
BASEINFOFILE="coverage-base.info"

# get into the correct directory
DETECTEDDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DETECTEDDIR/../

# error checking
set +e
GENHTML=`which lcov`
set -e
if [ "$GENHTML" = "" ] ; then
    echo "ERROR :: lcov is not in your path" 1>&2
    echo "      :: lcov source: http://downloads.sourceforge.net/ltp/lcov-1.10.tar.gz" 1>&2
    exit 1
fi

lcov -f -z --directory .                       # zero counters
lcov -f -c -i --directory . -o $BASEINFOFILE   # create baseline

