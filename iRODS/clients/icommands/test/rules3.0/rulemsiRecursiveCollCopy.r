myTestRule {
#Input parameters are:
#  Path of the destination collection
#  Path of the source collection
#Output parameter is:
#  Status
  writeLine("stdout","Files in destination collection before copy are:");
  msiExecStrCondQuery("SELECT DATA_NAME where COLL_NAME = '*DestColl'",*QOut);
  foreach(*QOut) {
    msiGetValByKey(*QOut,"DATA_NAME",*File);
    writeLine("stdout","*File");
  }
  msiRecursiveCollCopy(*DestColl,*SourceColl,*Status)
  writeLine("stdout","Files in destination collection after copy are:");
  msiExecStrCondQuery("SELECT DATA_NAME where COLL_NAME = '*DestColl'",*QOut);
  foreach(*QOut) {
    msiGetValByKey(*QOut,"DATA_NAME",*File);
    writeLine("stdout","*File");
  }
}
INPUT *SourceColl="/tempZone/home/rods/sub1", *DestColl="/tempZone/home/rods/sub2"
OUTPUT ruleExecOut
