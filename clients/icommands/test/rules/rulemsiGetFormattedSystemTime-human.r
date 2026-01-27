myTestRule {
#Input parameters are:
#  Optional format flag - human
#  Optional snprintf formatting for human format, using six inputs (year,month,day,hour,minute,second)
#    - the example below will print an ISO 8601 extended format datetimestamp
#Output parameter is:
#  Local system time
  msiGetFormattedSystemTime(*Out,"human","%d-%02d-%02dT%02d:%02d:%02d");
  writeLine("stdout",*Out);
}
INPUT null 
OUTPUT ruleExecOut
