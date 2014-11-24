#!/bin/bash

if [ $ORACLE_HOME ] ; then
    echo ${ORACLE_HOME}
else
    echo /usr/lib/oracle/11.2/client64
fi
