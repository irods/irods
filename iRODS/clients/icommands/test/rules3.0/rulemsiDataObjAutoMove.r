myTestRule {
# Input parameters are:
#   iRODS path of file to move
#   Leading collection name to be truncated from that iRODS path
#   iRODS path for destination collection
#   iRODS owner of the destination collection
#   Flag for whether checksum is computed, value is "true" to compute checksum
# Output from running the example is:
#   File /tempZone/home/rods/sub1/automove is moved to collection /tempZone/home/rods/ruletest
   msiSplitPath(*Path, *Coll, *File);
   msiDataObjAutoMove(*Path, *Coll, *Destination, *Owner, "true");
   writeLine("stdout","File *Path is moved to collection *Destination");
 }
INPUT *Path="/tempZone/home/rods/sub1/automove", *Destination="/tempZone/home/rods/ruletest", *Owner="rods"
OUTPUT ruleExecOut
