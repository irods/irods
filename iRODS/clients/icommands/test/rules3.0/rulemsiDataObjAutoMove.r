myTestRule {
# Input parameters are:
#   iRODS path of file to move
#   Leading collection name to be truncated from that iRODS path
#   iRODS path for destination collection
#   iRODS owner of the destination collection
#   Flag for whether checksum is computed, value is "true" to compute checksum
# Output from running the example is:
#   File /tempZone/home/rods/sub1/foo1 is moved to collection /tempZone/home/rods/ruletest/sub1
   msiSplitPath(*Path, *Coll, *File);
   msiDataObjAutoMove(*Path, *Coll, *Destination, *Owner, "true");
   writeLine("stdout","File *Path is moved to collection *Destination");
 }
INPUT *Path="/tempZone/home/rods/sub1/foo1", *Destination="/tempZone/home/rods/ruletest/sub1", *Owner="rods"
OUTPUT ruleExecOut
