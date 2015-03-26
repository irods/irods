myTestRule {
#Turn on audit trail in iRODS/server/icat/src/icatMidLevelRoutines.c
#Input parameters are:
#  Keyword to search for within audit comment field
#     read
#     delete
#     guest
#  Buffer to hold result
#Output parameter is:
#  Status
  msiGetAuditTrailInfoByKeywords(*Keyword,*Buf,*Status);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Keyword="%guest%"
OUTPUT ruleExecOut
