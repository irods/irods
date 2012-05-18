#!/bin/bash

# CentOS and Ubuntu
ODBC=`find /usr -name "psqlodbc*.so" 2> /dev/null | grep -v "w"`
NONEWLINES=`echo $ODBC | perl -ne 'chomp and print'`

# SuSE
if [ "$NONEWLINES" == "" ]; then
  ODBC=`find /usr -name "libodbcpsql.so" 2> /dev/null | grep -v "w"`
  NONEWLINES=`echo $ODBC | perl -ne 'chomp and print'`
fi

echo "$NONEWLINES"

