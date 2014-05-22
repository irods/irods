#!/bin/bash

SCRIPTNAME=`basename $0`
# check arguments
if [ $# -lt 1 -o $# -gt 2 ] ; then
  echo "Usage: $SCRIPTNAME username [nose parameter(s)]"
  exit 1
fi
if [ $# -eq 2 ] ; then
  NOSECMD="nosetests $2"
else
  NOSECMD="nosetests *.py"
fi

# check lcov version
LCOVVERSION=`lcov --version | awk '{print $4}'`
LCOVVERSIONMAJOR=`echo $LCOVVERSION | awk -F'.' '{print $1}'`
if (( "$LCOVVERSIONMAJOR" < "1" )); then
  echo "ERROR: LCOV is version [$LCOVVERSION] ... this script requires >= 1.8"
  exit 1
elif (( "$LCOVVERSIONMAJOR" == "1" )); then
  LCOVVERSIONMINOR=`echo $LCOVVERSION | awk -F'.' '{print $2}'`
  if (( "$LCOVVERSIONMINOR" < "8" )); then
    echo "ERROR: LCOV is version [$LCOVVERSION] ... this script requires >= 1.8"
    exit 1
  fi
fi

# prep
USERNAME=$1
D=`date +%Y%m%dT%H%M%S`
echo "DETECTED: username[$USERNAME] timestamp[$D]"

# sync
echo "----- SYNCING TEST FILES -----"
rsync -avz /home/$USERNAME/irods/tests/pydevtest/ ./

# coverage baseline
echo "----- GENERATING COVERAGE BASELINE -----"
/var/lib/irods/tests/coverage-pre.sh 2>&1 | tail -n1

# tests
echo "----- RUNNING TESTS -----"
echo "running: [$NOSECMD]"
$NOSECMD

# coverage report
if [ "$?" == "0" ] ; then
  echo "----- GENERATING COVERAGE REPORT -----"
  /var/lib/irods/tests/coverage-post.sh 2>&1 | tail -n4
  echo "--> coverageresults-$D"
  mv /var/lib/irods/coverageresults coverageresults-$D
  echo "----- SYNCING COVERAGE REPORT TO WWW -----"
  rsync -az ./coverageresults-$D/ /var/www/dev
  echo "http://127.0.0.1:8080/dev"
fi
