myTestRule {
#Input parameter is:
#  Flags in form keyword=value
#    objPath - the data object path to remove
#    replNum - the replica number to be removed
#    forceFlag= - flag to remove file without transferring to trash
#    irodsAdminRmTrash - flag to allow administrator to remove trash
#    irodsRmTrash - flag for user to remove trash
#Output parameter is:
#  Status
#Output from running the example is:
#  Replica number 1 of file /tempZone/home/rods/sub1/foo3 is removed
  msiDataObjUnlink("objPath=*SourceFile++++replNum=1",*Status);
  writeLine("stdout","Replica number 1 of file *SourceFile is removed");
}
INPUT *SourceFile="/tempZone/home/rods/sub1/foo3"
OUTPUT ruleExecOut
