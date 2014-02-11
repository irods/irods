#!/bin/bash

# Ubuntu (and CentOS 6.2+, via mysql-connector-odbc package)
ODBC=`find /usr -name "libmyodbc.so" 2> /dev/null | grep -v "w"`

# CentOS / SuSE
if [ "$ODBC" == "" ]; then
    # find 64bit version first, in case it exists alongside a 32bit version
    ODBC=`find /usr -name "libmyodbc*5.so" 2> /dev/null | grep "64"`
fi
if [ "$ODBC" == "" ]; then
    ODBC=`find /usr -name "libmysqlclient.so" 2> /dev/null`
fi
if [ "$ODBC" == "" ]; then
    ODBC=`find /usr -name "mysqlodbc*.so" 2> /dev/null`
fi

NONEWLINES=`echo $ODBC | perl -ne 'chomp and print'`
echo "$NONEWLINES"


