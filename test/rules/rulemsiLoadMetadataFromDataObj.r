myTestRule {
#Input parameter is:
#  Path name of file containing metadata
#    Format of file is
#    C-collection-name |Attribute |Value |Units
#    Path-name-for-file |Attribute |Value
#    /tempZone/home/rods/test/metadata-target.txt |Chicago |106 |Miles
#Output parameter is:
#  Status
  msiLoadMetadataFromDataObj(*Path,*Status);
  msiGetDataObjAVUs(*Filepath,*Buf);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Path="/tempZone/home/rods/test/load-metadata.txt", *Filepath="/tempZone/home/rods/test/metadata-target.txt"
OUTPUT ruleExecOut
