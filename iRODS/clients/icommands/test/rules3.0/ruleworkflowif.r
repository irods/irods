myTestRule {
#Workflow operator to evaluate conditional expression
#Input parameters are:
#  Logical expression that computes to TRUE or FALSE
#  Workflow to be executed defined within brackets
#  Else clause defined within brackets
#Output from running the example is:
#  0
    if(*A=="0") {
        writeLine("stdout", "0");
    } else {
        writeLine("stdout", "not 0");
    }
}
INPUT *A="0"
OUTPUT ruleExecOut

