#!/bin/bash

if [ $ORACLE_HOME ] ; then
    clntsh=$(find $ORACLE_HOME -name libclntsh.so)
    if [ ! -z ${clntsh} ] ; then
        if [ -e ${clntsh} ] ; then
            dirname ${clntsh}
        fi
    fi
else
    echo /usr/lib/oracle/11.2/client64
fi
