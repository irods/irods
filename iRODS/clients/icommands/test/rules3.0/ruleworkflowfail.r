myTestRule {
#Workflow function to cause immediate failure
#Output from running the example is:
#  ERROR: rcExecMyRule error.  status = -1091000 FAIL_ACTION_ENCOUNTERED_ERR
    if(*A=="fail") {
        fail;
    }
}
INPUT *A="fail"
OUTPUT ruleExecOut

