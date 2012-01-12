myTestRule {
# Input parameters are:
#   Tar file within iRODS that will have its files extracted
#   Collection where the extracted files will be placed
#   Resource where the extracted files will be written
# Output parameter:
#   Status flag for the operation
# Output from running the example is:
#  Extract files from a tar file into collection /tempZone/home/rods/ruletest/sub on resource demoResc 
   msiTarFileExtract(*File,*Coll,*Resc,*Status);
   writeLine("stdout","Extract files from a tar file *File into collection *Coll on resource *Resc");
 }
INPUT *File="/tempZone/home/rods/test/testcoll.tar", *Coll="/tempZone/home/rods/ruletest/sub", *Resc="demoResc"
OUTPUT ruleExecOut
