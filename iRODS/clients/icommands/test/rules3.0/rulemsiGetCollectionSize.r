myTestRule {
#Input parameter is:
#  Path of the collection
#Output parameters are:
#  Key-value pairs containing the results
#  Status
  msiGetCollectionSize(*Path,*Buf,*Status);
  msiPrintKeyValPair("stdout",*Buf);
}
INPUT *Path="/tempZone/home/rods/ruletest"
OUTPUT ruleExecOut
