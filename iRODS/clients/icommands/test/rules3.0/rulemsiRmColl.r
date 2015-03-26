myTestRule {
# Input parameters are:
#   Collection that will be removed
#   Flag controlling options in the form keyword=value
# Output parameter is:
#   Status flag for the operation
# Output from running the example is:
#  Removed collection /tempZone/home/rods/ruletest/subforrmcoll
   msiRmColl(*Coll,*Flag,*Status);
   writeLine("stdout","Removed collection *Coll");
 }
INPUT *Coll="/tempZone/home/rods/ruletest/subforrmcoll", *Flag="forceFlag="
OUTPUT ruleExecOut
