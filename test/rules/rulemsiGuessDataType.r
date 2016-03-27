myTestRule {
#Input parameter is:
#  Path of object
#Output parameters are:
#  Data type
#  Status
#Output from running the example is:
#  File /tempZone/home/rods/DBOtest/lt.pg has data type generic
  msiGuessDataType(*Path,*Type,*Status);
  writeLine("stdout","File *Path has data type *Type");
}
INPUT *Path="/tempZone/home/rods/DBOtest/notthere.mp3"
OUTPUT ruleExecOut
