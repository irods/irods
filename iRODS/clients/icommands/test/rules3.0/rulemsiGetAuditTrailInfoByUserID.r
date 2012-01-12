myTestRule {
#Input parameters are:
#  User identifier
#  Buffer for results
#Output parameter is:
#  Status
  msiExecStrCondQuery("SELECT USER_ID where USER_NAME = '*Person'",*QOut);
  foreach(*QOut) {
    msiGetValByKey(*QOut,"USER_ID",*PersonID);
    msiGetAuditTrailInfoByUserID(*PersonID,*Buf,*Status);
    writeBytesBuf("stdout",*Buf);
  }
}
INPUT *Person="rods"
OUTPUT ruleExecOut
