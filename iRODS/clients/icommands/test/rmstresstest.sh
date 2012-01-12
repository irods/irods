#!/bin/csh -e
set binDir = ../bin
if ("$2" == "" ) then
 echo "Usage: $0 <coll-start-number> <coll-count-number>"
 echo "  each collection of name  sttst.numwil be removed"
 echo " run this from icommands/test"
 exit 1
endif

set j = `expr $1 +  $2`
set i = $1

$binDir/icd
$binDir/ils
$binDir/icd stresstest
$binDir/ipwd
echo "------------------"
set startd = `date`
while ( $i < $j )
   echo "Timing irm -rf sttst.$i   --- 1000 files"
   time $binDir/irm -rf sttst.$i
   set i = `expr $i +  1`
end
$binDir/icd
echo "Size with ils -lr stresstest"
$binDir/ils -lr stresstest |wc
echo "------------------"
set endd = `date`
echo "Start: $startd"
echo "End  : $endd"
exit 0
