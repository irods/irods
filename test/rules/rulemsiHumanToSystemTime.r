myTestRule {
#Input parameter is:
#  Date in human readable form
#Output parameter is:
#  Time stamp in seconds since epoch
  msiGetSystemTime(*Date,"human");
  msiHumanToSystemTime(*Date,*Time);
  writeLine("stdout","Input date is *Date");
  writeLine("stdout","Time in unix seconds is *Time");
}
INPUT null
OUTPUT ruleExecOut
