myTestRule {
#Input parameters are:
#  Object identifier
#  Buffer for results
#Output parameter is:
#  Status
  msiSplitPath(*Path, *Coll, *File);
  msiExecStrCondQuery("SELECT DATA_ID where COLL_NAME = '*Coll' and DATA_NAME = '*File'",*QOut);
  foreach(*QOut) {
    msiGetValByKey(*QOut,"DATA_ID",*Objid);
    msiGetAuditTrailInfoByObjectID(*Objid,*Buf,*Status);
    writeBytesBuf("stdout",*Buf);
  }
}
INPUT *Path="/tempZone/home/rods/sub1/foo1"
OUTPUT ruleExecOut
