#!/bin/sh

## Copyright (c) 2009 Data Intensive Cyberinfrastructure Foundation. All rights reserved.
## For full copyright notice please refer to files in the COPYRIGHT directory
## Written by Jean-Yves Nief of CCIN2P3 and copyright assigned to Data Intensive Cyberinfrastructure Foundation

# This script is a template which must be updated if one wants to use the universal MSS driver.
# Your working version should be in this directory server/bin/cmd/univMSSInterface.sh.
# Functions to modify: syncToArch, stageToCache, mkdir, chmod, rm, stat
# These functions need one or two input parameters which should be named $1 and $2.
# If some of these functions are not implemented for your MSS, just let this function as it is.
#
 
# function for the synchronization of file $1 on local disk resource to file $2 in the MSS
syncToArch () {
	# <your command or script to copy from cache to MSS> $1 $2 
	# e.g: /usr/local/bin/rfcp $1 rfioServerFoo:$2
	return
}

# function for staging a file $1 from the MSS to file $2 on disk
stageToCache () {
	# <your command to stage from MSS to cache> $1 $2	
	# e.g: /usr/local/bin/rfcp rfioServerFoo:$1 $2
	return
}

# function to create a new directory $1 in the MSS logical name space
mkdir () {
	# <your command to make a directory in the MSS> $1
	# e.g.: /usr/local/bin/rfmkdir -p rfioServerFoo:$1
	return
}

# function to modify ACLs $2 (octal) in the MSS logical name space for a given directory $1 
chmod () {
	# <your command to modify ACL> $1 $2
	# e.g: /usr/local/bin/rfchmod $2 rfioServerFoo:$1
	return
}

# function to remove a file $1 from the MSS
rm () {
	# <your command to remove a file from the MSS> $1
	# e.g: /usr/local/bin/rfrm rfioServerFoo:$1
	return
}

# function to rename a file $1 into $2 in the MSS
mv () {
       # <your command to rename a file in the MSS> $1 $2
       # e.g: /usr/local/bin/rfrename rfioServerFoo:$1 rfioServerFoo:$2
       return
}

# function to do a stat on a file $1 stored in the MSS
stat () {
	# <your command to retrieve stats on the file> $1
	# e.g: output=`/usr/local/bin/rfstat rfioServerFoo:$1`
	error=$?
	if [ $error != 0 ] # if file does not exist or information not available
	then
		return $error
	fi
	# parse the output.
	# Parameters to retrieve: device ID of device containing file("device"), 
	#                         file serial number ("inode"), ACL mode in octal ("mode"),
	#                         number of hard links to the file ("nlink"),
	#                         user id of file ("uid"), group id of file ("gid"),
	#                         device id ("devid"), file size ("size"), last access time ("atime"),
	#                         last modification time ("mtime"), last change time ("ctime"),
	#                         block size in bytes ("blksize"), number of blocks ("blkcnt")
	# e.g: device=`echo $output | awk '{print $3}'`	
	# Note 1: if some of these parameters are not relevant, set them to 0.
	# Note 2: the time should have this format: YYYY-MM-dd-hh.mm.ss with: 
	#                                           YYYY = 1900 to 2xxxx, MM = 1 to 12, dd = 1 to 31,
	#                                           hh = 0 to 24, mm = 0 to 59, ss = 0 to 59
	echo "$device:$inode:$mode:$nlink:$uid:$gid:$devid:$size:$blksize:$blkcnt:$atime:$mtime:$ctime"
	return
}

#############################################
# below this line, nothing should be changed.
#############################################

case "$1" in
	syncToArch ) $1 $2 $3 ;;
	stageToCache ) $1 $2 $3 ;;
	mkdir ) $1 $2 ;;
	chmod ) $1 $2 $3 ;;
	rm ) $1 $2 ;;
	mv ) $1 $2 $3 ;;
	stat ) $1 $2 ;;
esac

exit $?
