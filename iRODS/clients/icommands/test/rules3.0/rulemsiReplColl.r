myTestRule {
#Input parameters are:
#  Collection path name
#  Destination resource name for replicas
#  Option string containing
#     all	- 
#     irodsAdmin - for administrator initiated replication
#     backupMode - will not throw an error if a good copy 
#                  already exists
#Output parameter is:
#  Status
# Output from running the example is:
#   Replicate collection /tempZone/home/rods/sub1 to location destRescName=testResc

  # Put a file in the collection
  msiDataObjPut(*Path,*Resource,"localPath=*LocalFile++++forceFlag=",*Status);
  msiSplitPath(*Path, *Coll, *File);

  #Replicate the collection
  msiReplColl(*Coll,*Dest,*Flag,*Status);
  writeLine("stdout","Replicate collection *Coll to location *Dest");
}
INPUT *Path="/tempZone/home/rods/sub1/foo1",*Resource="demoResc", *Dest="testResc", *LocalFile="foo1", *Flag="backupMode"
OUTPUT ruleExecOut
