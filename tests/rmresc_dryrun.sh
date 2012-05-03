#!/bin/bash

RESOURCENAME="BOOYAH"
THISHOST=`hostname`

echo "1. iadmin mkresc $RESOURCENAME \"unix file system\" cache $THISHOST /tmp/$USER/VaultToRemove"
iadmin mkresc $RESOURCENAME "unix file system" cache $THISHOST /tmp/$USER/VaultToRemove
echo

echo "2. iput -R $RESOURCENAME ./iRODS/COPYING"
iput -R $RESOURCENAME ./iRODS/COPYING 
echo

echo "3. ilsresc"
ilsresc
echo

echo "4. ils -L"
ils -L
echo

echo "5. iadmin rmresc --dryrun $RESOURCENAME"
fail=$(iadmin rmresc --dryrun $RESOURCENAME | grep FAIL)
echo $fail
echo

echo "6. ilsresc"
ilsresc
echo

echo "7. irm -f COPYING"
irm -f COPYING
echo

echo "8. ils -L"
ils -L
echo

echo "9. iadmin rmresc --dryrun $RESOURCENAME"
success=$(iadmin rmresc --dryrun $RESOURCENAME | grep SUCCESS)
echo $success
echo

echo "10. ilsresc"
ilsresc
echo

echo "Clean Up - 11. iadmin rmresc $RESOURCENAME"
iadmin rmresc $RESOURCENAME
echo

echo
echo

if [[ -n "$fail" && -n "$success" ]]; then
  echo "SUCCESS"
else
  echo "FAIL"
fi

