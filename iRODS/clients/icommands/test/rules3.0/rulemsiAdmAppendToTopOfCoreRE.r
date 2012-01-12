myTestRule {
#Input parameter is:
#  Core file name that will be prepended excluding the .re extension
#Output from running the example is:
#  Listing of the core.re file
  msiAdmAppendToTopOfCoreRE(*A);
  msiAdmShowCoreRE();
}
INPUT *A="core3"
OUTPUT ruleExecOut
