myTestRule {
#Workflow operation to loop until condition is false
#Input parameter is
#  Logical expression which evaluates to TRUE or FALSE
#  Workflow that is executed, defined within brackets
#Output from running the example is:
#  abcd
    *A = list("a","b","c","d");
    *B = "";
    *I=0;
    while(*I < 4) {
        *B = *B ++ elem(*A, *I);
        *I = *I + 1;
    }
    writeLine("stdout", *B);
}
INPUT null
OUTPUT ruleExecOut

