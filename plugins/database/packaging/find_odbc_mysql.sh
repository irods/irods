#!/bin/bash

# Ubuntu 10 and 12
ODBC=`find /usr -name "libmyodbc.so" 2> /dev/null`

# CentOS 6.x / SuSE 12
if [ "$ODBC" == "" ]; then
    # find 64bit version first, in case it exists alongside a 32bit version
    ODBC=`find /usr -name "libmyodbc5.so" 2> /dev/null | grep "64"`
fi
# CentOS 5.x / SuSE 11
if [ "$ODBC" == "" ]; then
    ODBC=`find /usr -name "libmyodbc3.so" 2> /dev/null | grep "64"`
fi
# Fallthrough
if [ "$ODBC" == "" ]; then
    ODBC=`find /usr -name "libmyodbc*.so" 2> /dev/null`
fi

NONEWLINES=`echo $ODBC | perl -ne 'chomp and print'`
echo "$NONEWLINES"

