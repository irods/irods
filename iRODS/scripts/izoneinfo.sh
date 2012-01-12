#!/bin/bash

# Usage is:
#	zone-info [-m |-mf]
#
# This shell script collects and displays some basic information on
# the local iRODS instance and optionally sends the information to
# DICE Research (the iRODS team).
#
# This should be run from the irods admin account on the ICAT-enabled
# host, with the i-commands in the path and 'iinit' run.
# If run from the build directory, it does not need to prompt for the 
# config.mk file location.
#

set -e   # exit if anything fails

userName=`whoami`
outFile="/tmp/izoneinfo.""$userName"".txt"
tmpFile="/tmp/zoneInfoTmpFile123.""$userName"

rm -f $outFile

#
# Record the starting date/time
#
date | tee -a $outFile

# Look for config.mk starting at this command's path and up one level,
# searching down 2 levels.  If not there, try same with current directory.
# If not there, ask for iRODS dir.

startDir=`echo $0 | sed s/izoneinfo.sh//g`
if [ -z $startDir ]; then
    startDir="./"
fi

set +e   # the ls will fail if file not there
config=`ls $startDir/config.mk 2> /dev/null`
if [ -z $config ]; then
   config=`ls  $startDir/config/config.mk 2> /dev/null`
fi
if [ -z $config ]; then
   config=`ls  $startDir../config.mk 2> /dev/null`
fi
if [ -z $config ]; then
   config=`ls  $startDir../config/config.mk 2> /dev/null`
fi
if [ -z $config ]; then
    startdir=`pwd`
    config=`ls $startDir/config.mk 2> /dev/null`
fi
if [ -z $config ]; then
   config=`ls  $startDir/config/config.mk 2> /dev/null`
fi
if [ -z $config ]; then
   config=`ls  $startDir/../config.mk 2> /dev/null`
fi
if [ -z $config ]; then
   config=`ls  $startDir/../config/config.mk 2> /dev/null`
fi

if [ -z $config ]; then
    echo "Could not find config.mk file (near this command or cwd)"
    printf "Please enter the full path of the iRODS build directory:"
    read startDir
    config=`ls $startDir/config.mk 2> /dev/null`
fi
if [ -z $config ]; then
   config=`ls  $startDir/config/config.mk 2> /dev/null`
fi
if [ -z $config ]; then
   config=`ls  $startDir/../config.mk 2> /dev/null`
fi
if [ -z $config ]; then
   config=`ls  $startDir/../config/config.mk 2> /dev/null`
fi

if [ -z $config ]; then
    echo "Can not find config.mk"
    exit 1
fi

set -e   # exit if anything fails


#
# Function to add commas to a number to make it more readable and also
# summarize the size (billions, etc).  Should work fine on most
# platforms: tested on Mac OS X, Linux Ubuntu, and Solaris.
#
function convertNumber {
    str=$1
    len=${#str}
    set +e
    extra=`expr $len % 3`
    groups=`expr $len / 3`
    set -e
    convertedNumber=${str:0:$extra}
    pos=$extra
    for ((i=1;i<=$groups;i+=1)); do
	if [ "$i" -eq 1 ]; then
	    if [ "$extra" -ne 0 ]; then
		convertedNumber="$convertedNumber,"
	    fi
	else
	    convertedNumber="$convertedNumber,"
	fi
	convertedNumber=$convertedNumber${str:$pos:3}
	pos=`expr $pos + 3`
    done
    set +e
    t=`expr $str / 1000000000000`
    if [ "$t" -gt 0 ]; then
	convertedNumber="$convertedNumber ($t trillion)"
    else
	b=`expr $str / 1000000000`
	if [ "$b" -gt 0 ]; then
	    convertedNumber="$convertedNumber ($b billion)"
	else
	    m=`expr $str / 1000000`
	    if [ "$m" -gt 0 ]; then
		convertedNumber="$convertedNumber ($m million)"
	    fi
	fi
    fi
    set -e
}


#
# Do a series of iquest commands to get basic information
#

zoneName=`iquest "%s" "select ZONE_NAME where ZONE_TYPE = 'local'"`
echo "Local zone is: $zoneName" | tee -a $outFile


set +e   # the query will fail if no remote zones are defined
tmp=`iquest "%s" "select ZONE_NAME where ZONE_TYPE = 'remote'" 2> /dev/null`
remoteZoneNames=`echo $tmp | sed 's/------------------------------------------------------------//g' | sed 's/ZONE_NAME = //g'`
set -e   # exit if anything fails
echo "Remote zones are: $remoteZoneNames" | tee -a $outFile

users=`iquest "%s" "select count(USER_ID) where USER_TYPE <> 'rodsgroup'"`
echo "Number of users: $users" | tee -a $outFile

groups=`iquest "%s" "select count(USER_NAME) where USER_TYPE = 'rodsgroup'"`
echo "Number of user groups: $groups" | tee -a $outFile

rescs=`iquest "%s" "select count(RESC_ID)"`
echo "Number of resources: $rescs" | tee -a $outFile

colls=`iquest "%s" "select count(COLL_ID)"`
convertNumber $colls
echo "Number of collections: $convertedNumber" | tee -a $outFile

dataObjs=`iquest "%s" "select count(DATA_ID)"`
convertNumber $dataObjs
echo "Number of data-objects: $convertedNumber" | tee -a $outFile

dataTotal=`iquest "%s" "select sum(DATA_SIZE)"`
convertNumber $dataTotal
echo "Total data size (bytes): $convertedNumber" | tee -a $outFile

rm -rf $tmpFile
dataDistribution=`iquest "%20s bytes in %12s files in '%s'" "SELECT sum(DATA_SIZE),count(DATA_NAME),RESC_NAME" > $tmpFile`
echo "Data distribution by resource:" | tee -a $outFile
cat $tmpFile | tee -a $outFile
rm -rf $tmpFile 

#
# Check the config file for various settings
#
echo Using config file: $config | tee -a $outFile

platform=`grep OS_platform $config | grep = | sed 's/_platform//g'`
echo $platform | tee -a $outFile

set +e
gsi=`grep GSI_AUTH $config | grep = | grep -v Uncomment | grep -v "#"`
set -e
if [ -z "$gsi" ]; then
  echo "no GSI" | tee -a $outFile
else
  echo "GSI enabled" | tee -a $outFile
fi

set +e
krb=`grep KRB_AUTH $config | grep = | grep -v Uncomment | grep -v "#"`
set -e
if [ -z "$krb" ]; then
  echo "no Kerberos" | tee -a $outFile
else
  echo "Kerberos enabled" | tee -a $outFile
fi

set +e
fuse=`grep IRODS_FS $config | grep = | grep -v Uncomment | grep -v "#"`
set -e
if [ -z "$fuse" ]; then
  echo "no FUSE" | tee -a $outFile
else
  echo "FUSE enabled" | tee -a $outFile
fi

modules=`grep MODULES $config | grep = | grep -v "#"`
echo $modules | tee -a $outFile

set +e
pgdb=`grep PSQICAT $config | grep = | grep -v "#" | grep -v "DPSQICAT"`
oradb=`grep ORAICAT $config | grep = | grep -v "#"`
mysqldb=`grep MYICAT $config | grep = | grep -v "#"`
set -e
if [ ! -z "$pgdb" ]; then
    echo "PostgreSQL ICAT" | tee -a $outFile
fi
if [ ! -z "$oradb" ]; then
    echo "Oracle ICAT" | tee -a $outFile
fi
if [ ! -z "$mysqldb" ]; then
    echo "MySQL ICAT" | tee -a $outFile
fi

#
# Get version, etc, from imiscsvrinfo
#
svrinfo=`imiscsvrinfo`
#printf "SvrInfo:" | tee -a $outFile
echo "SvrInfo:" | tee -a $outFile
echo $svrinfo | tee -a $outFile

#
# Record the ending date/time
#
date | tee -a $outFile

# 
# Optionally send this to DICE
printf "Please enter yes or y to send the above information to the iRODS team:"
read response
if [ ! -z $response ]; then
    if [ $response = "y" ] || [ $response = "yes" ]; then
	mailx -s zone-info schroeder@diceresearch.org < $outFile
	echo "Thank you for sharing this information with DICE Research"
    fi
fi

exit 0
