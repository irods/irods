myTestRule {
# Input parameters are:
#   Path
#   Flags specifying resource, and force option in format keyword=value
# Output parameter is:
#   File descriptor for the file
# Output from running the example is
#   Created and closed file /tempZone/home/rods/test/foo4
   msiDataObjCreate(*ObjB,*OFlagsB,*D_FD);
   writeLine("stdout","Created and closed file *ObjB");
   msiDataObjClose(*D_FD,*Status2);
}
INPUT *Resc="demoResc", *ObjB="/tempZone/home/rods/test/foo4", *OFlagsB="destRescName=demoResc++++forceFlag="
OUTPUT ruleExecOut
