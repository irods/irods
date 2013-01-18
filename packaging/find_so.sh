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
	# either 0 or 1 found
	elf_links[0]=`find /usr -name "$search_term" -print 2> /dev/null`
fi

# prepopulate the return variable with the proper values,
# in the case of a failure this will be overwritten with
# the error message
ret=${elf_links[0]}


# =-=-=-=-=-=-=-
# if there were candidates, but no binary file...
# pick first symlink, just because
if [[ ( $num_res -gt 0 ) && ( "$ctr" == "0" ) ]] ; then
	# return first one
	ret=`find /usr -name "$search_term" -print 2> /dev/null | head -n1`
fi

# =-=-=-=-=-=-=-
# if there are no candidates, the file is not on this machine.
# set return value accordingly
if [ "$ret" == "" ]; then
    echo "No shared object [$display_name] found.  Aborting." 1>&2
    ret="FAIL/FAIL"
fi

# =-=-=-=-=-=-=-
# if there is more than one binary file - look for 64bit only
if [ ${#elf_links[@]} -gt 1 ]; then
	# look for lib64 first
	num_64bit=$( find /usr -type f -name "$search_term" -print | grep "64" | xargs file | grep "ELF" 2> /dev/null | wc -l )
	if [ "$num_64bit" -eq 1 ]; then
		ret=`find /usr -type f -name "$search_term" -print | grep "64" 2> /dev/null`
		unset elf_links
		elf_links[0]=$ret
	fi
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
