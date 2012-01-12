#!/bin/bash

# Usage is:
#	buildboost
#
# This is a simple script layered on top of build_irods.sh to build
# the Boost library for use by iRODS, using the compressed boost_irods
# tar file contained in the iRODS distribution.  boost_irods is a
# subset of Boost which the RENCI/DICE team created from a standard
# Boost distribution, removing those aspects of it that are not needed
# within iRODS.  By using a smaller subset of Boost, the distribution
# file is much smaller and Boost builds much more quickly.  Also,
# since we are using a standard version of Boost, with no local
# modifications, it is useful to keep it as a separate binary file.

set -e   # exit if anything fails
boostFileZipped="boost_irods.tar.bz2"
boostFile="boost_irods.tar"
boostDir="boost_irods"

if [ ! -d $boostDir ]; then
    if [ ! -f $boostFile ]; then
	if [ ! -f $boostFileZipped ]; then
	    echo "Could not find $boostFileZipped"
	    exit 1
	fi
	set -x # echo the command line
	bzip2 -d $boostFileZipped
    fi
    set -x
    tar xf $boostFile
fi

set -x
cd $boostDir

./build_irods.sh
