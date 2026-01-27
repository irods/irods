myTestRule {
# Input parameters are:
#  Source collection path
#  Target collection path
#  Optional target resource
#  Optional synchronization mode:  IRODS_TO_IRODS
# Output parameter is:
#  Status of the operation
# Output from running the example is:
#  Synchronized collection /tempZone/home/rods/sub1 with collection /tempZone/home/rods/sub2
   msiCollRsync(*srcColl,*destColl,*Resource,"IRODS_TO_IRODS",*Status);
   writeLine("stdout","Synchronized collection *srcColl with collection *destColl");
}
INPUT *srcColl="/tempZone/home/rods/sub1", *destColl="/tempZone/home/rods/sub2", *Resource="demoResc"
OUTPUT ruleExecOut

