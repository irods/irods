#!/bin/sh

set -x 

myhost=`hostname`
mydate=`date`
mypwd=`pwd`

echo "$myhost:$mypwd $mydate irods1.sh starting"
../irods2.sh 2>&1
# remember error code
error1=$?

echo "$myhost:$mypwd $mydate irods1.sh ending (starting sleep), error1=($error1)"

cd $mypwd

# avoid rapid infinite loop, checking control files (set by hand):
# if /tmp/nosleep exists, sleep only very briefly, only once.
# if /tmp/sleep exists, sleep longer, checking occasionally.
if [ -f /tmp/nosleep ] ; then
  rm -f /tmp/nosleep
  sleep 10
else
  sleep 300
fi

if [ -f /tmp/sleep ] ; then
    sleep 600
    if [ -f /tmp/sleep ] ; then
        sleep 600
        if [ -f /tmp/sleep ] ; then
            sleep 600
            if [ -f /tmp/sleep ] ; then
                sleep 600
            fi
        fi
    fi
fi

# Another control file to wait indefinitely,
# for use when I'm doing manual testing
while [ -f /tmp/wait ] ; do
    sleep 60
done
exit $error1
