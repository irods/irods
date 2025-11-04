myTestRule {
#Input parameters are:
#  Key-value pair list
#  Key
#Output parameter is:
#  Value
#Output from running the example is:
#  List of file in the collection
  writeLine("stdout","List files in collection *Coll");
  msiExecStrCondQuery("SELECT DATA_NAME where COLL_NAME = '*Coll'",*QOut);
  foreach (*QOut) {
    msiGetValByKey(*QOut,"DATA_NAME",*File);
    writeLine("stdout","*File");
  }
}
INPUT *Coll="/tempZone/home/rods/sub1"
OUTPUT ruleExecOut
