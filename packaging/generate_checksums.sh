#!/bin/bash
#
# Script to generate checksums for a file or directory
#
# Output just goes to stdout - redirect accordingly
#

# make sure openssl is installed
OPENSSL=`which openssl`
if [[ "$?" != "0" ]] ; then
  echo "OpenSSL required" 1>&2
  exit 1
fi

echo ""
echo "Generated: "`date`

# is this a directory?
if [ -d "$1" ] ; then
  # if so, jump in and loop through it
  cd $1
  for FILENAME in *
  do
    if [ -d "$FILENAME" ] ; then
      echo ""
      echo "$FILENAME/ -- Skipping"
    else
      echo ""
      echo "$FILENAME"
      echo "  md5   "`openssl md5 $FILENAME | awk '{print $2}'`
      echo "  sha1  "`openssl sha1 $FILENAME | awk '{print $2}'`
    fi
  done
# else, a single file
else
  # is this a real file?
  if [ -f "$1" ] ; then
    FILENAME="$1"
    echo ""
    echo "$FILENAME"
    echo "  md5   "`openssl md5 $FILENAME | awk '{print $2}'`
    echo "  sha1  "`openssl sha1 $FILENAME | awk '{print $2}'`
  else
    echo "ERROR: [$1] is not a file or directory" 1>&2
    exit 1
  fi
fi

echo ""
