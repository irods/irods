myTestRule {
#Input parameters are:
#  Start time in unix seconds
#  Stop time in unix seconds
#  Buffer for results
#Output parameter is:
#  Status
#Output from example is audit trail from Startdate to current time
  msiGetIcatTime(*End,"unix");
  msiHumanToSystemTime(*Startdate,*Start);
  msiGetAuditTrailInfoByTimeStamp(*Start,*End,*Buf,*Status);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Startdate="2011-08-30.01:00:00"
OUTPUT ruleExecOut
