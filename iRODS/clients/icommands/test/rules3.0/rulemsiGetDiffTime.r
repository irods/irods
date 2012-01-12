myTestRule {
#Input parameters are:
#  Start date in system time in seconds
#  End date in system time in seconds
#  Optional format (human)
#Output parameter is:
#  Duration
  msiGetIcatTime(*Start,"unix");
  msiSleep("10","");
  msiGetIcatTime(*End,"unix");
  writeLine("stdout","Start time is *Start");
  msiGetDiffTime(*Start,*End,"",*Dur);
  writeLine("stdout","End time is *End");
  writeLine("stdout","Duration is *Dur");
}
INPUT null
OUTPUT ruleExecOut
