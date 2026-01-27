myTestRule {
#Input parameters are:
#  File ID
#  File path
#  Data Type to set
#Output parameter is:
#  Status
  msiSplitPath(*Path, *Coll, *File);
  msiExecStrCondQuery("SELECT DATA_ID where COLL_NAME = '*Coll' and DATA_NAME = '*File'", *QOut);
  foreach(*QOut) {
    msiGetValByKey(*QOut, "DATA_ID",*Objid);
    msiSetDataType(*Objid,*Path,*Datatype,*Status);
  }
  msiExecStrCondQuery("SELECT DATA_TYPE_NAME where COLL_NAME = '*Coll' and DATA_NAME = '*File'",*QOut1);
  foreach(*QOut1) {
    msiGetValByKey(*QOut1, "DATA_TYPE_NAME", *Dtype);
    writeLine("stdout","Data type retrieved is *Dtype");
  }
}
INPUT *Path="/tempZone/home/rods/sub1/foo1", *Datatype="text"
OUTPUT ruleExecOut
