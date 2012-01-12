myTestRule {
#Input parameter is:
#  Path name of file containing metadata
#    Format of file is
#    C-collection-name |Attribute |Value |Units
#    Path-name-for-file |Attribute |Value
#    /tempZone/home/rods/sub1/foo1 |Test |34
#Output parameter is:
#  Status
  msiLoadMetadataFromDataObj(*Path,*Status);
  msiGetDataObjAVUs(*Filepath,*Buf);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Path="/tempZone/home/rods/testcoll/rodsaccountavus", *Filepath="/tempZone/home/rods/sub1/foo1"
OUTPUT ruleExecOut
