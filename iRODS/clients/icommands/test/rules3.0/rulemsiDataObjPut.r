myTestRule {
# Input parameters are:
#   Data object path
#   Optional resource
#   Optional flags in form keyword=value
#     localPath
#     destRescName
#     all - to upload to all resources
#     forceFlag=
#     replNum - the replica number to overwrite
#     numThreads
#     filePath - the physical file path of the uploaded file on the server
#     dataType
#     verifyChksum=
# Output parameter is:
#   Status
# Output from running the example is:
#  File /tempZone/home/rods/sub1/foo1 is written to the data grid as foo1
  msiDataObjPut(*DestFile,*DestResource,"localPath=*LocalFile++++forceFlag=",*Status);
  writeLine("stdout","File *LocalFile is written to the data grid as *DestFile");
}
INPUT *DestFile="/tempZone/home/rods/sub1/foo1", *DestResource="demoResc", *LocalFile="foo1" 
OUTPUT ruleExecOut
