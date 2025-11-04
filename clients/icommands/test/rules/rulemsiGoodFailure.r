myTestRule {
#Workflow function to fail immediately with no recovery
#Output from running the example is:
#  ERROR: rcExecMyRule error.  status = -1088000 RETRY_WITHOUT_RECOVERY_ERR
    msiGoodFailure;
}
INPUT null
OUTPUT null

