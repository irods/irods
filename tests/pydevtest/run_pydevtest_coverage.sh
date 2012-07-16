#!/bin/bash

# get datestamp
D=`date +%Y%m%dT%H%M%S`

# sync
echo "----- SYNCING TEST FILES -----"
rsync -avz /home/tgr/e-irods/tests/pydevtest/ ./

# coverage baseline
echo "----- GENERATING COVERAGE BASELINE -----"
/var/lib/e-irods/tests/coverage-pre.sh 2>&1 | tail -n1

# tests
echo "----- RUNNING TESTS -----"
nosetests *.py

# coverage report
if [ "$?" == "0" ] ; then
  echo "----- GENERATING COVERAGE REPORT -----"
  /var/lib/e-irods/tests/coverage-post.sh 2>&1 | tail -n4
  echo "--> coverageresults-$D"
  mv /var/lib/e-irods/coverageresults coverageresults-$D
fi

