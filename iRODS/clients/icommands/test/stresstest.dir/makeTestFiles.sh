#!/bin/sh
#
# Simple script used as part of a stress test.
# Creates 999 copies of the template file, file.0.
# 
#

i=1
end=1000
echo "Creating 999 copies of file.0"
while [ $i -lt $end ]
  do
  `cp file.0 file.$i`
  i=`expr $i + 1`
done

exit 0
