myTestRule {
# Input parameters are:
#   Source data object file path
#   Destination object path
# Output parameter is:
#    Status
# Output from running the example is
#   File /tempZone/home/rods/sub1/for_versioning.txt copied to /tempZone/home/rods/sub1/SaveVersions
  msiStoreVersionWithTS(*SourceFile,*DestPath,*Status);
  writeLine("stdout","File *SourceFile copied to *DestPath");
}
INPUT *SourceFile="/tempZone/home/rods/sub1/for_versioning.txt", *DestPath="/tempZone/home/rods/sub1/SaveVersions"
OUTPUT ruleExecOut



