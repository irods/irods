myTestRule {
#Input parameter is:
#  Path of the file
#Output parameter is:
#  Buffer listing the AVU triplets
  msiGetDataObjPSmeta(*Path,*Buf);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Path="/tempZone/home/rods/sub1/foo2"
OUTPUT ruleExecOut
