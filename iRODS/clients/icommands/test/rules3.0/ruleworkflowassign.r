myTestRule {
#Workflow command to assign a value to a variable
#The assign microservice has been replaced with direct algebraic equations
#Output from running the example is:
#  Value assigned is assign
#
#    deprecated use:
#       assign(*A,*B);
#
#
    *A = *B;
    writeLine("stdout", "Value assigned is *A");
}
INPUT *B="assign"
OUTPUT ruleExecOut

