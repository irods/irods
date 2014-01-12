#!/bin/bash

# define display name and search term
display_name="$1"
search_term="$1*"

# =-=-=-=-=-=-=-
# determine how many hits we have for finding the file
num_res=$( find /usr -name "$search_term" -print 2> /dev/null | grep -v "vmware" | wc -l )

# =-=-=-=-=-=-=-
# count only binary ELF files
if [ $num_res -gt 1 ]; then
	ctr=0
	for file in `find /usr -name "$search_term" -print 2> /dev/null | grep -v "vmware"`
	do
		elfstatus=`file $file | grep ELF | wc -l`
		if [ "$elfstatus" == "1" ]; then
			elf_links[$ctr]=$file
       			ctr=$ctr+1
		fi
	done
else
	# either 0 or 1 found
	elf_links[0]=`find /usr -name "$search_term" -print 2> /dev/null | grep -v "vmware"`
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
	ret=`find /usr -name "$search_term" -print 2> /dev/null | grep -v "vmware" | head -n1`
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
ret=""
while read -r filepath; do
  val=`file $filepath | grep "ELF 64-bit"`
  if [ -n "$val" ]; then
    if [ -n "$ret" ]; then
      echo "Multiple versions of [$display_name] found, aborting installation" 1>&2
      echo -e "$( find /usr -name "$search_term" -print 2> /dev/null | grep -v "vmware" | xargs file )" 1>&2
      ret="FAIL/FAIL"
    else
      ret=$filepath
    fi
  fi
done <<< "$elf_links"

# =-=-=-=-=-=-=-
# if all is well, return the fullpath to this file
if [ $ret == "FAIL/FAIL" ]; then
  exit 3
fi
echo $ret
