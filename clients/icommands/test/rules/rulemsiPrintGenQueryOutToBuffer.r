myTestRule {
#Input parameters are:
#  GenQueryOut structure
#  C-style format string
#Output parameter is:
#  Buffer for result
  msiMakeGenQuery("DATA_ID, DATA_SIZE",*Condition,*GenQInp);
  msiExecGenQuery(*GenQInp, *GenQOut);
  msiPrintGenQueryOutToBuffer(*GenQOut,*Form,*Buf);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Coll = "/tempZone/home/rods/%%", *Condition="COLL_NAME like '*Coll'", *Form="For data-ID %-6.6s the data size is %-8.8s"
OUTPUT ruleExecOut
