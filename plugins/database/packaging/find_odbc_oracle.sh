#!/bin/bash

if [ $ORACLE_HOME ] ; then
    echo "${ORACLE_HOME}/lib/libsqora.so.11.1"
else
    echo "/usr/lib/oracle/11.2/client64/lib/libsqora.so.11.1"
fi
