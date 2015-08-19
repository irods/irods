#!/bin/bash
#
# Script to generate checksums for a file or directory
#
# Output just goes to stdout - redirect accordingly
#

checksum_file() {
    if [ ! -d "$1" ] ; then
        FILENAME="$1"
        echo ""
        echo "$FILENAME"
        echo "  md5   "`openssl md5 $FILENAME | awk '{print $2}'`
        echo "  sha1  "`openssl sha1 $FILENAME | awk '{print $2}'`
    fi
}

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
    for FILENAME in `find $1 | sort`
    do
        checksum_file $FILENAME
    done
# else, a single file
else
    checksum_file $1
fi

echo ""
