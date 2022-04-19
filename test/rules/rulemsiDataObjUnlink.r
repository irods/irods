myTestRule {
#Input parameter is:
#  Flags in form keyword=value
#    objPath - the data object path to remove
#    forceFlag= - flag to remove file without transferring to trash
#    irodsAdminRmTrash - flag to allow administrator to remove trash
#    irodsRmTrash - flag for user to remove trash
#Output parameter is:
#  Status
#Output from running the example is:
#  Data object /tempZone/home/rods/sub1/objunlink2 is moved to trash
  msiDataObjUnlink("objPath=*SourceFile++++forceFlag=",*Status);
  writeLine("stdout","Data object *SourceFile is now in the trash");
}
INPUT *SourceFile="/tempZone/home/rods/sub1/objunlink2"
OUTPUT ruleExecOut
