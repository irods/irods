myTestRule {
# Input parameters are:
#   Source data object path
#   Optional destination object path
#   Optional Object type
#      0 means data object
#      1 means collection
# Output parameter is:
#   Status
# Output from running the example is:
# The name of /tempZone/home/rods/sub1/foo1 is changed to /tempZone/home/rods/sub1/foo2
  msiDataObjRename(*SourceFile,*NewFilePath,"0",*Status);
  # To change the name of a collection, set the third input parameter to 1
  writeLine("stdout","The name of *SourceFile is changed to *NewFilePath");
}
INPUT *SourceFile="/tempZone/home/rods/sub1/foo1", *NewFilePath="/tempZone/home/rods/sub1/foo2" 
OUTPUT ruleExecOut
