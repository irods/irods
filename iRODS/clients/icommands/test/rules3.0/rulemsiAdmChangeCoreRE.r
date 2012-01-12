myTestRule {
#Input parameter is:
#  Name of file to replace the core.re file without the .re extension
#Output
#  Listing of the core.re file
  msiAdmChangeCoreRE(*A);
  msiAdmShowCoreRE();
}
INPUT *A="core-new-rule"
OUTPUT ruleExecOut
