myTestRule {
#Input parameter is:
#  Path of object
#Output parameter is:
#  Status
  msiApplyDCMetadataTemplate(*Path,*Status);
  msiGetDataObjPSmeta(*Path,*Buf);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Path="/tempZone/home/rods/sub1/dcmetadatatarget"
OUTPUT ruleExecOut
