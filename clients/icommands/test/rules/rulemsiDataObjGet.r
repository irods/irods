myTestRule {
# Input parameters are:
#   Data object path
#   Flags in form keyword=value
#    localPath
#    rescName
#    replNum
#    numThreads
#    forceFlag
#    verifyChksum
# Output parameter is
#   Status
# Output from running the example is:
#  File /tempZone/home/rods/sub1/foo1 is retrieved from the data grid
  msiSplitPath(*SourceFile,*Coll,*File);
  msiDataObjGet(*SourceFile,"localPath=./*File++++forceFlag=",*Status);
  writeLine("stdout","File *SourceFile is retrieved from the data grid");
}
INPUT *SourceFile="/tempZone/home/rods/sub1/foo1" 
OUTPUT ruleExecOut
