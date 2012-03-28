#!/bin/sh
CFG=`find . -name "irods.config"`
grep "IRODS_HOME =" $CFG | awk -F\' '{print $2}'
