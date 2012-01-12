myTestRule {
#Workflow command to break out of a loop
#Output from running the example is:
#  abc
    *A = list("a","b","c","d");
    *B = "";
    foreach(*A) {
        if(*A=="d") then {
            break;
        }
        *B = *B ++ *A;
    }
    writeLine("stdout", *B);
}
INPUT null
OUTPUT ruleExecOut

