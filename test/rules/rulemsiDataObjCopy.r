myTestRule {
# Input parameters are:
#   Source data object path
#   Optional destination object path
#   Optional flags in form keyword=value
#    destRescName
#    forceFlag=
#    numThreads
#    filePath="Physical file path of the uploaded file on the server"
#    dataType
#    verifyChksum=
# Output parameter is:
#    Status 
# Output from running the example is
#   File /tempZone/home/rods/sub1/foo1 copied to /tempZone/home/rods/sub2/foo1
  msiDataObjCopy(*SourceFile,*DestFile,"forceFlag=",*Status);
  writeLine("stdout","File *SourceFile copied to *DestFile");
}
INPUT *SourceFile="/tempZone/home/rods/sub1/foo1", *DestFile="/tempZone/home/rods/sub2/foo1" 
OUTPUT ruleExecOut
