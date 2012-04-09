#!/bin/bash

# =-=-=-=-=-=-=-
# error check to determine if there happens to be multiple output
# from the locate, meaning there are multiple versions of postgres
# located within the /usr region of interstellar space.
num_res=$( find /usr -name "plpgsql.so" -print 2> /dev/null | grep usr | wc -l )
if [ $num_res -gt 1 ]; then
	echo "Multiple versions of postgres found, aborting installation"
	echo -e `find /usr -name "plpgsql.so" -print 2> /dev/null `
	exit "FAIL" 
fi

# =-=-=-=-=-=-=-
# if all is well, compose a path for psql
result=$( find /usr -name "plpgsql.so" -print 2> /dev/null | grep usr )
result=`dirname $result`
result=`dirname $result`
result="$result/bin/psql"

echo $result
