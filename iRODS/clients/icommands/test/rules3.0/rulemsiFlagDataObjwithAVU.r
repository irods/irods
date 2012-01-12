myTestRule {
#Requires acPostProcForModifyAVUMetadata rule in core.re of form
#acPostProcForModifyAVUMetadata(*Option,*ItemType,*ItemName,*AName,*AValue)||nop|nop
#Input parameters are:
#  Source path
#  Flag name
#Output parameter is:
#  Status
#Output from running the example is:
# Metadata attribute called State1 is added to  /tempZone/home/rods/sub1/foo1
  msiFlagDataObjwithAVU(*Source,*Flag,*Status);
  writeLine("stdout","Metadata attribute called *Flag is added to *Source");
}
INPUT *Source="/tempZone/home/rods/sub1/foo1", *Flag="State1"
OUTPUT ruleExecOut
