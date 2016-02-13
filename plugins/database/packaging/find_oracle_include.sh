#!/bin/bash

if [ ${ORACLE_HOME} ] ; then
    oci=`find $ORACLE_HOME -name "oci.h"`
    if [ ! -z ${oci} ] ; then
        if [ -e ${oci} ] ; then
            dirname $oci
	    else
		echo "/usr/include/oracle/11.2/client64"
	    fi
    else
	echo "/usr/include/oracle/11.2/client64"
    fi
else
    echo "/usr/include/oracle/11.2/client64"
fi
