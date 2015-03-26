myTestRule {
#Output
#  Buffer holding list of microservices in form Key=Value
#Output from running the example is:
#    List of microservices that are enabled
  msiListEnabledMS(*Buf);
  writeKeyValPairs("stdout",*Buf,":");
}
INPUT null
OUTPUT ruleExecOut
