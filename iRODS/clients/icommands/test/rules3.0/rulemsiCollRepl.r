myTestRule {
# This microservice is deprecated
# Input parameters are:
#   Collection that will be replicated, it must contain at least one file
#   Target resource in keyword-value form
# Output parameter is:
#   Status of operation
# Output from running the example is:
#   Replicate collection /tempZone/home/rods/sub1 to location destRescName=testResc

  # Put a file in the collection
  msiDataObjPut(*Path,*Resource,"localPath=*LocalFile++++forceFlag=",*Status);
  msiSplitPath(*Path, *Coll, *File);
  msiCollRepl(*Coll, *RepResource, *status);
  writeLine("stdout","Replicate collection *Coll to location *RepResource");
}
INPUT *RepResource="destRescName=testResc", *Path="/tempZone/home/rods/sub1/foo1", *Resource="demoResc", *LocalFile="foo1"
OUTPUT ruleExecOut
