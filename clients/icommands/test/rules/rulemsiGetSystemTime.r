myTestRule {
#Input parameters are:
#  Time type "icat" or "unix" returns time in seconds
#            "human" returns date in Year-Month-Day.Hour:Minute:Second
#Output parameter is:
#  Time value for local system
  msiGetSystemTime(*Start,"unix");
  msiGetSystemTime(*End,"human");
  writeLine("stdout","Time in seconds is *Start");
  writeLine("stdout","Time human readable is *End");
}
INPUT null
OUTPUT ruleExecOut
