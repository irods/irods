myTestRule {
#Input parameter is:
#  Path of file
#Output parameters are:
#  Data object ID
#  Status
#Output from running the example is:
#  The data ID of file /tempZone/home/rods/sub1/foo1 is 10077
  msiIsData(*Path,*DataID,*Status);
  writeString("stdout","The data ID of file *Path is ");
  writePosInt("stdout",*DataID);
  writeLine("stdout","");
}
INPUT *Path="/tempZone/home/rods/sub1/foo1"
OUTPUT ruleExecOut
