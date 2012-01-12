myTestRule {
# Input parameters are:
#   Collection that will be bundled into a tar file
#   Resource where the tar file will be stored
# Output parameter is:
#   Status flag for the operation
#   The file is stored under the /tempZone/bundle/home/rods directory in iRODS
# Output from running the example is
#  Create tar file of collection /tempZone/home/rods/test on resource testResc
  msiPhyBundleColl(*Coll, *Resc, *status);
  writeLine("stdout","Create tar file of collection *Coll on resource *Resc");
}
INPUT *Coll="/tempZone/home/rods/test", *Resc="testResc"
OUTPUT ruleExecOut
