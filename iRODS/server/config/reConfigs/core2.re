acDataObjCreate {
  acSetCreateConditions;
  acDOC; }
acSetCreateConditions{ msiGetNewObjDescriptor ::: recover_msiGetNewObjDescriptor;
  acSetResourceList; }
acDOC {
  msiPhyDataObjCreate ::: recover_msiPhyDataObjCreate;
  acRegisterData;
  msiCommit ::: msiRollback; }
acSetResourceList { TTTTmsiSetResourceList; }
acRegisterData {msiRegisterData ::: msiRollback; }
