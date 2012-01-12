acServerMonPerf {
#This microservice invokes a command in iRODS/server/bin/cmd
#    irodsServerMonPerf     - a perl script to get monitoring information
  delay("<PLUSET>30s</PLUSET>< EF>1h</EF>") {
    msiServerMonPerf("default","default");
 }
}
INPUT null
OUTPUT ruleExecOut
