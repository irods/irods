#!/bin/bash -ex

# version to use
ASTYLEVERSION=2.05.1

# variables
IRODSROOT=$( dirname $( cd $(dirname $0) ; pwd -P ) )
ASTYLEBIN=$IRODSROOT/meta/astyle/build/gcc/bin/astyle
GZFILE=astyle.tar.gz

# clean up any prior runs
cd $IRODSROOT/meta
rm -rf $GZFILE
rm -rf astyle

# download and build astyle
FTPFILE="ftp://ftp.renci.org/pub/irods/external/astyle_"$ASTYLEVERSION"_linux.tar.gz"
curl -o $GZFILE -z $GZFILE $FTPFILE
tar zxf $GZFILE
cd astyle/build/gcc
make -j4

# run astyle
source $IRODSROOT/meta/astyleparams
cd $IRODSROOT
$ASTYLEBIN ${ASTYLE_PARAMETERS} --exclude=external -v -r *.hpp *.cpp

# clean up this run
cd $IRODSROOT/meta
rm -rf $GZFILE
rm -rf astyle

