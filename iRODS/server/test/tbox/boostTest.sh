#!/bin/sh
# Simple script to build boost-irods, configure irods to use it, rebuild
# irods with it, and run the test suite.
set -x
date
cd /tbox/IRODS_BUILD/iRODS
buildboost.sh
date

cd config
mv config.mk config.mk.old
cat config.mk.old | sed 's/# USE_BOOST=1/USE_BOOST=1/g' > config.mk
cd ..

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/tbox/IRODS_BUILD/iRODS/boost_irods/lib

make
date

./irodsctl restart
./irodsctl testWithoutConfirmation
res=$?
date
exit $res
