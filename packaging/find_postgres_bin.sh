#!/bin/bash

# =-=-=-=-=-=-=-
# determine how many hits we have for finding psql
num_res=$( find /usr -name "psql" -print 2> /dev/null | grep usr | wc -l )

# =-=-=-=-=-=-=-
# filter out all symlink candidates
if [ $num_res -gt 1 ]; then
	ctr=0
	for file in `find /usr -name "psql" -print 2> /dev/null`
	do
		if [ -L $file ]; then
			continue
		else
			not_links[$ctr]=$file
			ctr=$ctr+1
		fi
	done
else
	not_links[0]=`find /usr -name "psql" -print 2> /dev/null`
fi

# prepopulate the return variable with the proper values,
# in the case of a failure this will be overwritten with
# the error message
ret=${not_links[0]}


# =-=-=-=-=-=-=-
# if there are still more than one candidate, then something terrible has happened
# we shall bail and eschew all responsibility, silly human.
if [ ${#not_links[@]} -gt 1 ]; then
	echo "Multiple versions of postgres found, aborting installation"
	echo -e `find /usr -name "psql" -print 2> /dev/null `
	ret="FAIL/FAIL"
fi


# =-=-=-=-=-=-=-
# if all is well, return the path to postgres binaries
echo `dirname $ret`
