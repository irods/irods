myTestRule {
#Input parameter is:
#  Path of collection
#Output parameters are:
#  Buffer for results in form key-value pairs
#  Status
  msiGetCollectionContentsReport(*Path,*KVPairs,*Status);
  writeKeyValPairs("stdout",*KVPairs, " : ");
}
INPUT *Path="/tempZone/home/rods"
OUTPUT ruleExecOut
