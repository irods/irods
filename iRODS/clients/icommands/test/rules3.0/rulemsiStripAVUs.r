myTestRule {
#Input parameters are:
#  Path to data file
#  Optional flag - not used 
#Output parameter is:
#  Status

  # Set an AVU
  msiFlagDataObjwithAVU(*Path,"State",*Status);

  # Retreive the AVUs
  msiGetDataObjPSmeta(*Path,*Buf);
  writeLine("stdout","Metadata on *Path is:");
  writeBytesBuf("stdout",*Buf);
  writeLine("stdout","");

  # Delete the AVUs
  msiStripAVUs(*Path,"",*Status);

  # Verify the AVUs have been deleted
  msiGetDataObjPSmeta(*Path,*Buf);
  writeLine("stdout","Metadata has been removed on *Path");
  writeBytesBuf("stdout",*Buf);
  writeLine("stdout","");
}
INPUT *Path="/tempZone/home/rods/sub1/foo2"
OUTPUT ruleExecOut
