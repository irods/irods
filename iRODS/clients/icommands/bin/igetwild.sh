#!/bin/sh
#

# Simple script that does some wildcard processing to iget irods files
# with names with entered patterns in the names (at the beginning,
# end or middle).
#
# For example:
# igetwild.sh /newZone/home/rods .txt e
# will get each irods file that ends with '.txt' in the collection.
#
#igetwild.sh /newZone/home/rods .txt e
#   foo.txt                         0.000 MB | 0.164 sec | 0 thr |  0.000 MB/s
#   foo2.txt                        0.000 MB | 0.101 sec | 0 thr |  0.000 MB/s

# Include the collection name, the pattern of the dataObject
# names and b,e,or m on the command line, to avoid prompts for them.

# This is just a interim partial solution while we develop a more
# comprehensive approach to handling wild cards.

#
#set -x

tmpFile1="/tmp/tmpIgetScriptFile1.$$"
tmpFile2="/tmp/tmpIgetScriptFile2.$$"
tmpFile3="/tmp/tmpIgetScriptFile3.$$"

if [ $1 ]; then
  collName=$1
else
  echo "This simple script allows you to iget a set of files"
  echo "using wild-card type matching on file names."
  echo "This is just a interim partial solution while we develop a more"
  echo "comprehensive approach, but may be useful to some."
  echo "You can enter the arguments on the command-line if you'd prefer."
  echo "Also see 'igetwild.sh -h'."
  printf "Please enter the collection to iget from: "
  read collName
fi

if [ $collName = "-h" ]; then
  echo "Get one or more iRODS files using wildcard characters."
  echo "Usage: igetwild.sh collection pattern b|m|e  (beginning, middle, or end)"
  echo "Will prompt for missing items."
  exit 0
fi

if [ $2 ]; then
  matchPattern=$2
else
  printf "Please enter the pattern in the file names to iget (just the text): "
  read matchPattern
fi

matchPosition=e
if [ $3 ]; then
  matchPosition=$3
else
  printf "Should this be at the end, beginning, or middle of the names? [e b or m] : "
  read matchPosition
fi

rm -f $tmpFile1 $tmpFile2 $tmpFile3

if [ $matchPosition = "e" ]; then
  iquest "select DATA_NAME where COLL_NAME = '$collName' and DATA_NAME like '%$matchPattern'" > $tmpFile1
fi
if [ $matchPosition = "b" ]; then
  iquest "select DATA_NAME where COLL_NAME = '$collName' and DATA_NAME like '$matchPattern%'" > $tmpFile1
fi
if [ $matchPosition = "m" ]; then
  iquest "select DATA_NAME where COLL_NAME = '$collName' and DATA_NAME like '%$matchPattern%'" > $tmpFile1
fi



grep DATA_NAME $tmpFile1 > $tmpFile2

`cat $tmpFile2 | sed 's#DATA_NAME = #iget -v '$collName'/#g' > $tmpFile3`

sh < $tmpFile3

rm -f $tmpFile1 $tmpFile2 $tmpFile3
