myTestRule {
#Input parameters are:
#  Buffer where the string is written
#    stdout
#    stderr
#    serverLog
#  String that is written
#Output from running the example is:
#  string
    writeString(*Where, *StringIn);
    writeLine(*Where,"cheese");
}
INPUT *Where="stdout", *StringIn="string"
OUTPUT ruleExecOut

