myTestRule {
# Input parameters are:
#   Path
#   Flags specifying resource, and force option in format keyword=value
# Output parameter is:
#   File descriptor for the file
# Output from running the example is
#   Created and closed file /tempZone/home/rods/test/foo4
   msiDataObjCreate(*ObjB,*OFlagsB,*D_FD);
   msiDataObjClose(*D_FD,*Status2);
   writeLine("stdout","Created and closed file *ObjB");
 }
INPUT *Resc="demoResc", *ObjB="/tempZone/home/rods/test/foo4", *OFlagsB="destRescName=demoResc++++forceFlag="
OUTPUT ruleExecOut
