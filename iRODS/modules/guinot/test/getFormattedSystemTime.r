getFormattedSystemTime {
#Input parameters are:
#  Optional format flag - human
#  Optional snprintf formatting for human format, using six inputs (year,month,day,hour,minute,second)
#    - the example below will print an ISO 8601 extended formatdatetimestamp
#Output parameter is:
#  Local system time
 msiGetFormattedSystemTime(*Out,"human","%d-%d-%dT%d:%d:%d");
 writeLine("stdout",*Out);
}
INPUT null
OUTPUT ruleExecOut