myTestRule {
#Requires acPostProcForModifyAVUMetadata rule in core.re of form
#acPostProcForModifyAVUMetadata(*Option,*ItemType,*ItemName,*AName,*AValue)||nop|nop
#Input parameters are:
#  Source path
#  Flag name
#Output parameter is:
#  Status
#Output from running the example is:
# Metadata attribute called State3 is added to  /tempZone/home/rods/test/ERAtestfile.txt
  msiFlagDataObjwithAVU(*Source,*Flag,*Status);
  writeLine("stdout","Metadata attribute called *Flag is added to *Source");
}
INPUT *Source="/tempZone/home/rods/test/ERAtestfile.txt", *Flag="State3"
OUTPUT ruleExecOut
