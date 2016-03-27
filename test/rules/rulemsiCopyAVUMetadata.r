myTestRule {
#Requires acPostProcForModifyAVUMetadata rule in core.re of form
#acPostProcForModifyAVUMetadata(*Option,*ItemType,*ItemName,*AName,*AValue)||nop|nop
#Input parameters are:
#  Source path
#  Destination path
#Output parameter is:
#  Status
#Output from running the example is:
#  /tempZone/home/rods/sub1/mdcopysource, State1
#  Metadata copied from /tempZone/home/rods/sub1/mdcopysource to /tempZone/home/rods/sub1/mdcopydest
  writeLine("stdout"," *Source, *Flag");
  msiFlagDataObjwithAVU(*Source,*Flag,*Status);
  msiCopyAVUMetadata(*Source,*Dest,*Status);
  writeLine("stdout","Metadata copied from *Source to *Dest");
}
INPUT *Source="/tempZone/home/rods/sub1/mdcopysource", *Flag="State1",*Dest="/tempZone/home/rods/sub1/mdcopydest"
OUTPUT ruleExecOut
