#!/bin/bash

echo "1. iadmin mkresc BOOYA \"unix file system\" cache centos1.irods.renci.org /usr/share/eirods/Vault2"
iadmin mkresc BOOYA "unix file system" cache centos1.irods.renci.org /usr/share/eirods/Vault2
echo

echo "2. iput -R BOOYA ./iRODS/COPYING"
iput -R BOOYA ./iRODS/COPYING 
echo

echo "3. ilsresc"
ilsresc
echo

echo "4. ils -L"
ils -L
echo

echo "5. iadmin rmresc --dryrun BOOYA"
fail=$(iadmin rmresc --dryrun BOOYA | grep FAIL)
echo $fail
echo

echo "6.  ilsresc"
ilsresc
echo

echo "7. irm -f COPYING"
irm -f COPYING
echo

echo "8. ils -L"
ils -L
echo

echo "9. iadmin rmresc --dryrun BOOYA"
success=$(iadmin rmresc --dryrun BOOYA | grep SUCCESS)
echo $success
echo

echo "10.  ilsresc"
ilsresc
echo

echo "Clean Up - iadmin rmresc BOOYA"
iadmin rmresc BOOYA
echo

echo
echo

if [[ -n "$fail" && -n "$success" ]]; then
	echo "SUCCESS"
else
	echo "FAIL"
fi

