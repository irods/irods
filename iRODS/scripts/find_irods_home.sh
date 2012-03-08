#!/bin/sh
CFG=`find . -name "irods.config"`
grep "IRODS_HOME =" $CFG | sed -e "s/\([^']*'\)//" -e "s/'[^']*$//" 
