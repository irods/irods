myTestRule {
#Input parameters are:
#  Path of collection to bundle
#  Path of bundle that will be created
#  Optional target resource
#Output parameter is:
#  Status
#Output from running the example is:
#  Bundle file created as a tar file
  msiStructFileBundle(*Path,*Bundle,*Resc,*Status);
  msiGuessDataType(*Bundle,*Type,*Status);
  writeLine("stdout","Bundle file created as a *Type");
}
INPUT *Path="/tempZone/home/rods/sub1", *Bundle="/tempZone/home/rods/test/sub1.tar", *Resc="testResc"
OUTPUT ruleExecOut  
