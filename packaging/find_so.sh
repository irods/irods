#!/bin/bash

# define display name and search term
display_name="$1"
search_term="$1*"

# =-=-=-=-=-=-=-
# determine how many hits we have for finding the file
num_res=$( find /usr -name "$search_term" -print 2> /dev/null | wc -l )

# =-=-=-=-=-=-=-
# count only binary ELF files
if [ $num_res -gt 1 ]; then
	ctr=0
	for file in `find /usr -name "$search_term" -print 2> /dev/null`
	do
		elfstatus=`file $file | grep ELF | wc -l`
		if [ "$elfstatus" == "1" ]; then
			elf_links[$ctr]=$file
       			ctr=$ctr+1
		fi
	done
else
	elf_links[0]=`find /usr -name "$search_term" -print 2> /dev/null`
fi

# prepopulate the return variable with the proper values,
# in the case of a failure this will be overwritten with
# the error message
ret=${elf_links[0]}


# =-=-=-=-=-=-=-
# if there are no candidates, the file is not on this machine.
# set return value accordingly
if [ "$ret" == "" ]; then
    echo "No shared object [$display_name] found.  Aborting." 1>&2
    ret="FAIL/FAIL"
fi

# =-=-=-=-=-=-=-
# if there is still more than one candidate, we cannot continue.
# set return value accordingly
if [ ${#elf_links[@]} -gt 1 ]; then
	echo "Multiple versions of [$display_name] found, aborting installation" 1>&2
	echo -e `find /usr -name "$search_term" -print 2> /dev/null ` 1>&2
	ret="FAIL/FAIL"
fi


# =-=-=-=-=-=-=-
# if all is well, return the fullpath to this file
echo $ret
