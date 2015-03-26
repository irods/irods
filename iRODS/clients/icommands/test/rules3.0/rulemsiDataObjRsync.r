rulemsiDataObjRsync {
#Input parameters are:
#  Data object path
#  Optional flag for mode
#    IRODS_TO_IRODS
#    IRODS_TO_COLLECTION
#  Optional storage resource
#  Optional target collection
#Output parameters are:
#  status
#Output from running the example is:
# Source object /tempZone/home/antoine/source.txt synchronized with destination object /tempZone/home/antoine/dest.txt
  msiDataObjRsync(*sourceObj,"IRODS_TO_IRODS",*destResc,*destObj,*status);
  writeLine("stdout","Source object *sourceObj synchronized with destination object *destObj");
  
  writeLine("stdout","status = *status");
  
}
INPUT *sourceObj="/tempZone/home/rods/synctest/source.txt",*destResc="demoResc",*destObj="/tempZone/home/rods/synctest/dest.txt"
OUTPUT ruleExecOut
