#!/bin/bash

# =-=-=-=-=-=-=-
# find path to msvc home
#echo "find irods home"
cwd=`pwd`
cd ../../../iRODS
irods_home=`./scripts/find_irods_home.sh`
#echo "irods_msvc_home - $irods_msvc_home"
irods_msvc_home="$irods_home/server/bin"
#echo "irods_msvc_home - $irods_msvc_home"
cd $cwd

# =-=-=-=-=-=-=-
# copy msvc bin to proper directory and set ownership
#echo "copy & chown"
cp ./libeirods_msvc_test.so $irods_msvc_home
chown eirods:eirods $irods_msvc_home/libeirods_msvc_test.so

# =-=-=-=-=-=-=-
# run rule to properly exec msvc 
#echo "run msvc"
irule -F ./run_eirods_msvc_test.r &> /dev/null

# =-=-=-=-=-=-=-
# verify that it loaded and ran properly

msvc_run=$(grep "eirods_msvc_test :: 1 2 3" $irods_home/server/log/rodsLog* )
#echo "msvc_run - $msvc_run"

# =-=-=-=-=-=-=-
# check that rule didnt load twice
dupe_load=$(grep "load_microservice_plugin :: loaded eirods_msvc_test" $irods_home/server/log/rodsLog*)
#echo "dupe_load - $dupe_load"

# =-=-=-=-=-=-=-
# run a rule to try to load a nonexistent msvc
#echo "run missing"
irule -F ./run_eirods_missing_msvc_test.r &> /dev/null

# =-=-=-=-=-=-=-
# test to see that it failed properly
miss_msvc=$(grep "load_microservice_plugin :: failed to load microservice plugin: /var/lib/eirods/iRODS/server/bin/libmissing_micro_service.so" /var/lib/eirods/iRODS/server/log/rodsLog* )
#echo "miss_msvc - $miss_msvc"


# =-=-=-=-=-=-=-
# run a non-plugin rule 
#echo "show core re"
irule -F ./rulemsiAdmShowCoreRE.r &> ./reout.txt
if [[ ! -s ./eout.txt ]]; then
	non_plugin="YES"
else
	non_plugin="NO"
fi
#echo "non_plugin - $non_plugin"

# =-=-=-=-=-=-=-
# verify that non-plugin rule ran fine
if [[ "x$msvc_run" != "x" && "x$dupe_load" != "x" && "x$miss_msvc" != "x" && $non_plugin == "YES" ]]; then
	echo SUCCESS
else
	echo FAIL
fi




