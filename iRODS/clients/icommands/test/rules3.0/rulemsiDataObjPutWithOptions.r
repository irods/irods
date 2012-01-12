myTestRule {
# Input parameters are:
#   Data object path
#   Optional resource
#   Optional local file path
#   Optional flag forceFlag
#   Optional flag all - to overwrite all copies
# Output parameter is:
#   Status
# Output from running the example is:
# File foo1 is written into the data grid as file /tempZone/home/rods/sub1/foo1
  msiDataObjPutWithOptions(*DestFile,*DestResource,*LocalFile,"forceFlag","",*Status);
  writeLine("stdout","File *LocalFile is written into the data grid as file *DestFile");
}
INPUT *DestFile="/tempZone/home/rods/sub1/foo1", *DestResource="demoResc", *LocalFile="foo1" 
OUTPUT ruleExecOut
