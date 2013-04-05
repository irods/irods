#!/bin/bash

###################################################################
#
#  A basic test to show functionality of an HPSS resource
#
#  Requires eirods ownership of the keytab:
#  $ sudo chown eirods:eirods /var/hpss/etc/hpss.eirods.keytab
#
###################################################################

# display commands
set -x

# exit on failure
set -e

# show resource list
iadmin lr

# create hpss resource
RESCNAME="basictest_hpss_resc"
HOSTNAME=`hostname`
iadmin mkresc $RESCNAME hpss $HOSTNAME:/VaultPath "user=eirods;keytab=/var/hpss/etc/hpss.eirods.keytab"

# show resource
iadmin lr $RESCNAME

# iput
TESTFILE="basichpsstestfile"
DATETIME=`date`
DATETIME=${DATETIME//[[:space:]]} # remove spaces
DATETIME=${DATETIME//:}           # remove colons
ls > $TESTFILE
iput -R $RESCNAME $TESTFILE $TESTFILE-$DATETIME

# ils
ils -AL

# show resource - see object count
iadmin lr $RESCNAME

# iget
iget $TESTFILE-$DATETIME

# diff
diff $TESTFILE $TESTFILE-$DATETIME

# rm locally
rm $TESTFILE
rm $TESTFILE-$DATETIME

# irm
irm -f $TESTFILE-$DATETIME

# remove resource
iadmin rmresc $RESCNAME

# show resource list
iadmin lr

# done
exit 0

