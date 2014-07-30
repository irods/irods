#!/bin/bash

# find whatever psql is in the current user's path
ret=`which psql`
# if none found report a failure, do not exit 1
if [ "$ret" == "" ]; then
    echo "No postgres [psql] found.  Aborting." 1>&2
    ret="FAIL/FAIL"
fi
# return the bin directory holding psql
echo `dirname $ret`
# exit cleanly
exit 0
