#!/bin/bash -e

# SDL Usage # concurrent tests
# 

#set -x 

verboseflag=1
numoferrors=1
OS=Darwin


TMP_DIR=TMP
thistest=putget
testdirs="zerofiles smallfiles bigfiles"
#testdirs="zerofiles"

irodshome="../../"

numzerofiles=100
numsmallfiles=100
numbigfiles=10

thisdir=`pwd`
echo thisdir: $thisdir


usage () {
	echo "Usage: $0 <numtests> [bigfilesize-in-KB]"
	exit 1
}

# Make data directories of varying file numbers and sizes
makefiles () {

	for dir in $testdirs; do
		test -d $dir || mkdir $dir

	case "$dir" in
		"zerofiles")
			echo "making zerofiles"
			for ((i=1;i<=$numzerofiles;i+=1)); do
				touch zerofiles/zerofile$i
			done
		;;

		"smallfiles")
			echo "making smallfiles"
			for ((i=1;i<=$numsmallfiles;i+=1)); do
				echo "abcdefghijklmnopqrstuvwxyz1234567890" > smallfiles/smallfile$i
			done
		;;

		"bigfiles")
			cd src; make clean; make; cd ..
			echo "making bigfiles ($inSize KB)"
			$thisdir/src/writebigfile $inSize
			for ((i=1;i<=$numbigfiles;i+=1)); do
				cp bigfile bigfiles/bigfile$i				
			done
			/bin/rm bigfile
		;;
	esac

	done
}

now=`date`
echo "Beginning poundtest at $now"
echo "Beginning poundtest at $now" >> $0.log


if [ "$1" = "" ]; 
then 
	usage 
fi


# make our test directories
inSize=$2
makefiles

firstProcCount=`ps | wc -l`
testIds=""

numberOfTests=0;
for testdir in $testdirs; do

	i=0
	while [ $i -lt $1 ]; do
		
		case "$testdir" in
		"zerofiles")
			testid=$thistest-$numzerofiles-$testdir-`date "+%Y%m%d%H%M%S"`
		;;
		"smallfiles")
			testid=$thistest-$numsmallfiles-$testdir-`date "+%Y%m%d%H%M%S"`
		;;
		"bigfiles")
			testid=$thistest-$numbigfiles-$testdir-`date "+%Y%m%d%H%M%S"`
		;;

		esac;
		
		numtest=`expr $i + 1`
		echo $numtest of $1:$thistest $testid began at `date`
		sh -ex $irodshome/clients/concurrent-test/$thistest $testid $testdir > $testid.irods 2>&1 &
		# check for startup failure
		if [ "$?" -ne 0 ]; then
			echo "$testid FAILED, exiting"
			exit 3
		fi

		testIds=`echo $testIds $testid`

		i=`expr $i +  1`
		numtest=`expr $i +  1`
		numberOfTests=`expr $numberOfTests + 1`
		
		sleep 3

		now=`date`
		echo "Test $testid started at $now" >> $0.log
		echo "Test $testid started at $now"
	done

done

#Wait for the background tests to complete
procCount=`expr $firstProcCount + 1`
while [ $procCount -gt $firstProcCount ]; do
    sleep 5
    procCount=`ps | wc -l`
done

numberOtTests=`expr $numtest - 1`
echo All $numberOfTests of $thistest completed as of `date`
echo All $numberOfTests of $thistest completed as of `date` >> $0.log

#Check the results
for testid in $testIds; do
    result=`tail -1 $testid.irods | grep 'exit 0' | wc -l`
    if [ $result -eq 1 ]; then
	echo "$testid ended Successfully" >> $0.log
	echo "$testid ended Successfully"
    else
	echo "$testid Failed" >> $0.log
	echo "$testid Failed"
	exit 4
    fi
done

# Do one irmtrash (not multiple) or else can get errors
../../clients/icommands/bin/irmtrash

#Clean up locally
for testdir in $testdirs; do
	/bin/rm -rf $testdir
done
/bin/rm -rf TMP Isub1 Isub2

now=`date`
echo "$0 completed successfully at $now" >> $0.log
echo "$0 completed successfully at $now"

exit 0
