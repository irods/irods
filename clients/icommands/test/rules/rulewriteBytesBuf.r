myTestRule {
#Input Parameters are:
#  Location for write (stdout, stderr)
#  String buffer

  # Make a query
  msiMakeGenQuery("DATA_ID, DATA_SIZE",*Condition,*GenQInp);

  # Issue the query and retrieve query result
  msiExecGenQuery(*GenQInp, *GenQOut);

  # Convert result to a buffer
  msiPrintGenQueryOutToBuffer(*GenQOut,*Form,*Buf);

  # write the result buffer
  writeBytesBuf("stdout",*Buf);
}
INPUT *Coll="/tempZone/home/rods/sub1", *Condition="COLL_NAME like '*Coll'", *Form="For data-ID %-6.6s the data size is %-8.8s"   
OUTPUT ruleExecOut
