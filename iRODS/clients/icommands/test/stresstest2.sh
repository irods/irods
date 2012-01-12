#!/bin/csh -e
set binDir = ../bin
set defResc = data1at14Resc
set defResc2 = data2at14Resc
$binDir/icd
$binDir/ils
$binDir/imkdir stresstest
$binDir/icd stresstest
$binDir/ipwd
echo "------------------"
set startd = `date`
   echo "Timing iput -R $defResc  ../test/bigData.1GB"
   time $binDir/iput -R $defResc  ../test/bigData.1GB 
   echo "Timing irepl -R $defResc2  bigData.1GB"
   time $binDir/irepl -R $defResc2  bigData.1GB 
   echo "Timing iget bigData.1GB -  to /dev/null"
   time $binDir/iget bigData.1GB - > /dev/null
$binDir/icd
echo "Size with ils -lr stresstest"
$binDir/ils -lr stresstest |wc
echo "Timing ils -lr stresstest"
time $binDir/ils -L stresstest 
echo "Timing irm -rf stresstest"
time $binDir/irm -rf stresstest
echo "------------------"
set endd = `date`
echo "Start: $startd"
echo "End  : $endd"
exit 0
