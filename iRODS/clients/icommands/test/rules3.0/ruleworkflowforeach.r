myTestRule {
#Workflow operator to iterate over a list
#Input parameter is:
#  List
#  Workflow executed within brackets
#Output from running the example is:
#  abcd
    *A = list("a","b","c","d");
    *B = "";
    foreach(*A) {
        *B = *B ++ *A;
    }
    writeLine("stdout", *B);
}
INPUT null
OUTPUT ruleExecOut

