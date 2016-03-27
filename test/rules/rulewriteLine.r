myTestRule {
#Input parameters are:
#  Name of output buffer
#    stdout
#    stderr
#    serverLog
#  String to write
#Output from running the example is:
#  line
    writeLine(*Where, *StringIn);
}
INPUT *Where="stdout", *StringIn="line"
OUTPUT ruleExecOut

