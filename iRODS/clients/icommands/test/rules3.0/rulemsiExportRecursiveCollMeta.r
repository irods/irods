myTestRule {
#Input parameter is:
#  Path of target collection
#Output parameter is:
#  Buffer holding results
#Output from running the example lists:
#  file-name|metadata-name|metadata-value
  msiExportRecursiveCollMeta(*Path,*Buf);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Path="/tempZone/home/rods/sub1"
OUTPUT ruleExecOut
