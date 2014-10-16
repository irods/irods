#!/bin/bash

# =-=-=-=-=-=-=-
# count only binary ELF files for mysql hits
ctr=0
for file in `find /usr -name "mysql" -print 2> /dev/null`
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
# if there are no candidates, there is no mysql on this machine.
# set return value accordingly
if [ "$ret" == "" ]; then
    echo "No MySQL [mysql] found.  Aborting." 1>&2
    ret="FAIL/FAIL"
    exit 1
fi

# =-=-=-=-=-=-=-
# iterate over the array and test for --help to determine
# the appropriate executable
for i in ${elf_links[@]}
do
    help_val=`$i --help 2>&1`
    if [ $? == 0 ]; then
        `echo $help_val | grep Oracle > /dev/null 2>&1`
        if [ $? == 0 ]; then
            ret=$i
            break
        fi
    fi

done

if [ "$ret" == "" ]; then
    echo "No MySQL [mysql] found.  Aborting." 1>&2
    ret="FAIL/FAIL"
    exit 1
fi


# =-=-=-=-=-=-=-
# if there is still more than one candidate, we cannot continue.
# set return value accordingly
#if [ ${#elf_links[@]} -gt 1 ]; then
#	echo "Multiple versions of mysql found, aborting installation" 1>&2
#	echo -e `find /usr -name "mysql" -print 2> /dev/null ` 1>&2
#	ret="FAIL/FAIL"
#fi


# =-=-=-=-=-=-=-
# if all is well, return the path to postgres binaries
echo `dirname $ret`
