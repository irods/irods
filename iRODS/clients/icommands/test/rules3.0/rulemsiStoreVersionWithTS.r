myTestRule {
# Input parameters are:
#   Source data object file path
#   Destination object path
# Output parameter is:
#    Status
# Output from running the example is
#   File /tempZone/home/rods/put_test.txt copied to /tempZone/home/rods/SaveVersions
  msiStoreVersionWithTS(*SourceFile,*DestPath,*Status);
  writeLine("stdout","File *SourceFile copied to *DestPath");
}
INPUT *SourceFile="/tempZone/home/rods/put_test.txt", *DestPath="/tempZone/home/rods/SaveVersions"
OUTPUT ruleExecOut
