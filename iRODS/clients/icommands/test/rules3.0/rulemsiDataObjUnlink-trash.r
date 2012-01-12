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
#  Files in trash are removed
  msiDataObjUnlink("objPath=*Path++++irodsAdminRmTrash=",*Status);
  writeLine("stdout","File in trash are removed");
}
INPUT *Path="/tempZone/trash/home/rods/sub2"
OUTPUT ruleExecOut
