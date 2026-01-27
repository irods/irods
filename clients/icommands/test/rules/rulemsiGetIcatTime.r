myTestRule {
#Input parameters are:
#  Time type (icat/unix or human) in seconds
#Output parameter is:
#  Time value
  msiGetIcatTime(*Start,"unix");
  msiGetIcatTime(*End,"human");
  writeLine("stdout","Time in seconds is *Start");
  writeLine("stdout","Time human readable is *End");
}
INPUT null
OUTPUT ruleExecOut
