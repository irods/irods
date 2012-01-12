myTestRule {
#Input parameter is:
#  Path of collection
#Output parameters are:
#  Collection ID
#  Status
#Output from running the example is:
#  The collection ID of /tempZone/home/rods is 10008
  msiIsColl(*Path,*CollID,*Status);
  writeString("stdout","The collection ID of *Path is ");
  writePosInt("stdout",*CollID);
  writeLine("stdout","");
}
INPUT *Path="/tempZone/home/rods"
OUTPUT ruleExecOut
