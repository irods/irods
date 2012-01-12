myTestRule {
#Requires acPostProcForModifyAVUMetadata rule in core.re of form
#acPostProcForModifyAVUMetadata(*Option,*ItemType,*ItemName,*AName,*AValue)||nop|nop
#Input parameters are:
#  Source path
#  Destination path
#Output parameter is:
#  Status
#Output from running the example is:
#  /tempZone/home/rods/sub1/foo1, State1
#  Metadata copied from /tempZone/home/rods/sub1/foo1 to /tempZone/home/rods/sub1/foo2
  writeLine("stdout"," *Source, *Flag");
  msiFlagDataObjwithAVU(*Source,*Flag,*Status);
  msiCopyAVUMetadata(*Source,*Dest,*Status);
  writeLine("stdout","Metadata copied from *Source to *Dest");
}
INPUT *Source="/tempZone/home/rods/sub1/foo1", *Flag="State1",*Dest="/tempZone/home/rods/sub1/foo2"
OUTPUT ruleExecOut
