myTestRule {
#Workflow operator to iterate through variable
#Input parameters are:
#  Loop initiation
#  Loop termination
#  Loop increment
#  Workflow in brackets
#Output from running the example is:
#  abcd
    *A = list("a","b","c","d");
    *B = "";
    for(*I=0;*I<4;*I=*I+1) {
        *B = *B ++ elem(*A, *I);
    }
    writeLine("stdout", *B);
}
INPUT null
OUTPUT ruleExecOut

