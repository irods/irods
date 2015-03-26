myTestRule {
# Input parameters are:
#   Data object path
#   Local file path
#   Source resource
# Output parameter is:
#   Status
# Output from executing the example is:
#  File /tempZone/home/rods/sub1/foo1 is retrieved from the data grid and written to foo1
  msiSplitPath(*SourceFile,*Coll,*File);
  msiDataObjGetWithOptions(*SourceFile,"./*File",*Resource,*Status);
  writeLine("stdout","File *SourceFile is retrieved from the data grid and written to *File");
}
INPUT *SourceFile="/tempZone/home/rods/sub1/foo1", *Resource="demoResc" 
OUTPUT ruleExecOut
