#!/bin/sh
#
# Simple script used as part of a load test (run by multiple users).
# Runs iput, ils and iget of multiple files and verifies a little.
# 
# Creates and destroys local and irods directories called 'tmpdir' or
# whatever is $tmpDir and also creates/destroys local directory tmpdir2.
#
# Assumes i-commands in path and iinit has been run.
#

# functions ################################################
function makeSmallFiles {
# Make a directory with input-arg small files
    rm -rf $tmpDir
    mkdir $tmpDir
    cd $tmpDir
    i=0
    echo "creating $1 small files in $tmpDir"
    while [ $i -lt $1 ]
      do
      `ls -l > f$i`
      i=`expr $i + 1`
    done
    cd ..
}

function makeLargeFiles {
# Make a directory with input-arg large files of size input2
    rm -rf $tmpDir
    mkdir $tmpDir
    cd $tmpDir
    i=0
    echo "creating $1 large files of size $2"
    size=`expr $2 / 2`
    while [ $i -lt $1 ]
      do
      `yes | head -$size > f$i`
      i=`expr $i + 1`
    done
    cd ..
}

function cleanUp {
    echo "removing test files on local disk"
    rm -rf $tmpDir
    rm -rf tmpdir2
}

function loadTest {
# test with input-arg number of files, which are in the $tmpDir
# directory.
    echo "putting files"
    iput -r $tmpDir
    echo "listing files"
    count=`ils -l $tmpDir | wc -l`
    i=`expr $count - 1`
    if [ $i -ne $1 ]; then
	echo "ils shows wrong number of files"
	exit 1
    fi

    rm -rf tmpdir2
    mkdir tmpdir2
    cd tmpdir2
    echo "getting files"
    iget -r $tmpDir
    cd ..

    echo "comparing files"
    diff -r $tmpDir tmpdir2/$tmpDir

    echo "removing irods files"
    irm -rf $tmpDir
    echo "removing trash"
    irmtrash
}

# end fuctions ############################################


set -e # error exit on any command errors
set +x # do not echo commands

tmpDir="tmpdir"

makeSmallFiles 500
loadTest 500
cleanUp

makeLargeFiles 4 50000000
loadTest 4
cleanUp

echo "Success"

exit 0
