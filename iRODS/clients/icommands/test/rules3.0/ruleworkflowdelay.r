myTestRule {
#Workflow operator to execute a given workflow at a delayed specification
#Input parameters are:
#  Delay condition composed from tags
#    EA     - host where the execution if performed
#    ET     - Absolute time when execution is done
#    PLUSET - Relative time for execution
#    EF     - Execution frequency
#    Workflow specified within brackets
#Output from running the example is:
#  exec
#Output written to the iRODS/server/log/reLog log file:
# writeLine: inString = Delayed exec 
  delay("<PLUSET>30s</PLUSET>") {
    writeLine("serverLog","Delayed exec");
  }
  writeLine("stdout","exec");
}
INPUT null
OUTPUT ruleExecOut
