acServerMonPerf {
#This microservice invokes a command in iRODS/server/bin/cmd
#    irodsServerMonPerf     - a perl script to get monitoring information
    	msiServerMonPerf("default","default");
    	msiServerMonPerf("verbose","default");
}
INPUT null
OUTPUT ruleExecOut
