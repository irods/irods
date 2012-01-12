myTestRule {
#Input parameter is:
#  Name of file to replace the core.irb file
  msiAdmChangeCoreIRB(*A);
}
INPUT *A="core-orig.irb"
OUTPUT ruleExecOut
