#!/bin/sh
#
# Script to test the hasher
RESULT=0
TESTFILE=foo
#
# Generate test file
rm ${TESTFILE}
cp `which bash` ${TESTFILE}
#
EXEC=./hasher_test
SHA256SUM=`sha256sum ${TESTFILE} | cut -f1 -d" "`
MD5SUM=`md5sum ${TESTFILE} | cut -f1 -d" "`
OUTPUT=`${EXEC} ${TESTFILE}`
MD5OUT=`echo ${OUTPUT} | cut -f1 -d" "`
SHA256OUT=`echo ${OUTPUT} | cut -f2 -d" "`
if [ "${MD5SUM}" != "${MD5OUT}" ]
then
    printf "ERROR: MD5 output \"%s\" does not match md5sum output \"%s\"\n" ${MD5OUT} ${MD5SUM}
    RESULT=255
fi
if [ "${SHA256SUM}" != "${SHA256OUT}" ]
then
    printf "ERROR: SHA256 output \"%s\" does not match sha256sum output \"%s\"\n" ${SHA256OUT} ${SHA256SUM}
    RESULT=255
fi
exit ${RESULT}
