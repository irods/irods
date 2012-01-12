myTestRule {
#Input parameters are:
#  Data object path
#  Optional flag for mode
#    IRODS_TO_IRODS
#    IRODS_TO_COLLECTION
#  Optional storage resource
#  Optional target collection
#Output parameters are:
#  Status
#Output from running the example is:
#  The file /tempZone/home/rods/sub1/foo2 is synchronized onto the logical data object path /tempZone/home/rods/rules
  msiDataObjRsync(*SourceFile,"IRODS_TO_IRODS",*DestResource,*DestPathName,*Status);
  writeLine("stdout","The file *SourceFile is synchronized onto the logical data object path *DestPathName");
}
INPUT *SourceFile="/tempZone/home/rods/sub1/foo2",*DestResource="testResc",*DestPathName="/tempZone/home/rods/rules"
OUTPUT ruleExecOut
