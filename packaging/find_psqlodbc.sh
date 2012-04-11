#!/bin/bash
ODBC=`find /usr -name "psqlodbc*.so" 2> /dev/null | grep -v "w"`
NONEWLINES=`echo $ODBC | perl -ne 'chomp and print'`
echo $NONEWLINES
