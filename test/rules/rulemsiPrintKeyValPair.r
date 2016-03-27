myTestRule {
#Input parameters are:
#  Location where data is written
#    stdout
#    stderr
#  Structure holding key-value pairs
#Example lists metadata for an input file path
  msiSplitPath(*Path,*Coll,*File);
  msiExecStrCondQuery("SELECT *Meta where COLL_NAME = '*Coll' and DATA_NAME = '*File'",*QOut);
  foreach(*QOut) {msiPrintKeyValPair("stdout",*QOut);}
}
INPUT *Path="/tempZone/home/rods/sub1/foo1", *Meta="DATA_TYPE_NAME"
OUTPUT ruleExecOut 
