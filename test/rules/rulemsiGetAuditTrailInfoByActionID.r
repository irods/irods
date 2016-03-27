myTestRule {
#Turn on audit trail in iRODS/server/icat/src/icatMidLevelRoutines.c
#Input parameters are defined in:
# iRODS/server/icat/include/icatDefines.h
#  Identifier of action
#    1000   - grant access
#    2040   - delete user
#    2061   - delete collection
#    2076   - modify user password
#    2120   - modify access control on file
#  Buffer name to be used by microservice
#Output parameter is:
#  Status
  msiGetAuditTrailInfoByActionID(*Id,*Buf,*Status);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Id="2040"
OUTPUT ruleExecOut
