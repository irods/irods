myTestRule {
#Input parameter is:
#  Path of file
#Output parameter is:
#  Buffer listing AVUs for the file in XML format
  msiGetDataObjAVUs(*Path,*Buf);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Path="/tempZone/home/rods/test/ERAtestfile.txt"
OUTPUT ruleExecOut
