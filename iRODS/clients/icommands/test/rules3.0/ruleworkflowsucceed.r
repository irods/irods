myTestRule {
#Workflow operation to cause rule to immediately succeed
#Output from running the example is:
#  succeed
    if(*A == "succeed") {
        writeLine("stdout", "succeed");
        succeed;
    } else {
        fail;
    }
}
INPUT *A="succeed"
OUTPUT ruleExecOut

