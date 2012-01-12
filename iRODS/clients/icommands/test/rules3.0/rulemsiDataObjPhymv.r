myTestRule {
# Input parameters are:
#   Data object path
#   Optional destination resource name
#   Optional source resource name
#   Optional replica number
#   Optional keyword for IRODS_ADMIN
# Output parameters are:
#   Status
# Output from running the example is:
#  Replica number 0 of file /tempZone/home/rods/sub1/foo1 is moved from resource demoResc to resource testResc
  msiDataObjPhymv(*SourceFile,*DestResource,*SourceResource,*ReplicaNumber,"null",*Status);
  writeLine("stdout","Replica number *ReplicaNumber of file *SourceFile is moved from resource *SourceResource to resource *DestResource");
}
INPUT *SourceFile="/tempZone/home/rods/sub1/foo1", *DestResource="testResc", *SourceResource="demoResc", *ReplicaNumber="0"
OUTPUT ruleExecOut
