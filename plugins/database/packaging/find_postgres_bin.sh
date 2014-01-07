#!/bin/bash

# =-=-=-=-=-=-=-
# count only binary ELF files for psql hits
ctr=0
for file in `find /usr -name "psql" -print 2> /dev/null`
do
	elfstatus=`file $file | grep ELF | wc -l`
	if [ "$elfstatus" == "1" ]; then
		elf_links[$ctr]=$file
		ctr=$ctr+1
	fi
done

# prepopulate the return variable with the proper values,
# in the case of a failure this will be overwritten with
# the error message
ret=${elf_links[0]}


# =-=-=-=-=-=-=-
# if there are no candidates, there is no psql on this machine.
# set return value accordingly
if [ "$ret" == "" ]; then
    echo "No postgres [psql] found.  Aborting." 1>&2
    ret="FAIL/FAIL"
fi

# =-=-=-=-=-=-=-
# if there is still more than one candidate, we cannot continue.
# set return value accordingly
if [ ${#elf_links[@]} -gt 1 ]; then
	echo "Multiple versions of postgres found, aborting installation" 1>&2
	echo -e `find /usr -name "psql" -print 2> /dev/null ` 1>&2
	ret="FAIL/FAIL"
fi


# =-=-=-=-=-=-=-
# if all is well, return the path to postgres binaries
echo `dirname $ret`
