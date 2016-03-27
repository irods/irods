myTestRule {
#Input parameters are:
#  Path of collection
#  Flag
#    recursive  - to get ACLs for sub-collections and files
#Output parameter is:
#  Buffer holding the result
  msiGetCollectionACL(*Path,"recursive",*Buf);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Path="/tempZone/home/rods"
OUTPUT ruleExecOut
