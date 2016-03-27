myTestRule {
#Input parameter is:
#  Path of the file
#Output parameter is:
#  Buffer with ACLs
  msiGetDataObjACL(*Path,*Buf);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Path="/tempZone/home/rods/test/ERAtestfile.txt"
OUTPUT ruleExecOut
