myTestRule {
# Input parameters are:
#   Tar file path name that will be created
#   Collection that will be turned into a tar file
#   Resource where the tar file will be stored
#   Flag controlling options in form keyword=value
# Output from running the example is:
#  Created tar file /tempZone/home/rods/test/testcoll.tar for collection /tempZone/home/rods/ruletest/sub on resource demoResc
   msiTarFileCreate(*File,*Coll,*Resc,*Flag);
   writeLine("stdout","Created tar file *File for collection *Coll on resource *Resc");
 }
INPUT *File="/tempZone/home/rods/test/testcoll.tar", *Coll="/tempZone/home/rods/ruletest/sub", *Resc="demoResc", *Flag=""
OUTPUT ruleExecOut
