myTestRule {
#Input parameter is:
#  Path of collection
#Output parameter is:
#  Buffer holding the result
  msiGetCollectionPSmeta(*Path,*Buf);
  writeLine("stdout","Testing for no metadata on collection");
  writeBytesBuf("stdout",*Buf);
}
INPUT *Path="/tempZone/home/rods/sub2"
OUTPUT ruleExecOut
