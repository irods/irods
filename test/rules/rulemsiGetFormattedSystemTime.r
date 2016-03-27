myTestRule {
#Input parameters are:
#  Optional format flag - human
#  Optional snprintf formatting for human format, using six inputs (year,month,day,hour,minute,second)
#Output parameter is:
#Local system time
  msiGetFormattedSystemTime(*Out,"null","null");
  writeLine("stdout",*Out);
}
INPUT null 
OUTPUT ruleExecOut
