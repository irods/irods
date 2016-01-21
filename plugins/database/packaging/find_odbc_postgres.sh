#!/bin/bash

SEARCHPATH=$HOME
if [ "$1" != "" ] ; then
    SEARCHPATH="$1"
    SEARCHPATH=$(echo $(cd $SEARCHPATH; pwd) )
fi

# MacOSX Run In Place Support
ODBC=`find $SEARCHPATH -name "libodbcpsql.so" 2> /dev/null | grep -v "src/interfaces"`

# Ubuntu (and CentOS 6.2+, via postgresql-odbc package)
if [ "$ODBC" == "" ]; then
    ODBC=`find /usr -name "psqlodbc*.so" 2> /dev/null | grep -v "w\.so"`
fi
# CentOS / SuSE
if [ "$ODBC" == "" ]; then
    # find 64bit version first, in case it exists alongside a 32bit version
    ODBC=`find /usr -name "libodbcpsql.so" 2> /dev/null | grep "64"`
fi
if [ "$ODBC" == "" ]; then
    ODBC=`find /usr -name "libodbcpsql.so" 2> /dev/null`
fi
if [ "$ODBC" == "" ]; then
    ODBC=`find /usr -name "psqlodbc*.so" 2> /dev/null`
fi


NONEWLINES=`echo $ODBC | perl -ne 'chomp and print'`
echo "$NONEWLINES"

