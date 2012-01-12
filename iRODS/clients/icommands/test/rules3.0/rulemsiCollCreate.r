myTestRule {
# Input parameters are:
#  Collection that will be created
#  Flag specifying whether to create parent collection
#   Value of 1 means create parent collection
# Output parameter:
#  Result status for the operation
#Output from running the example
#  Create collection /tempZone/home/rods/ruletest/sub1
#  Collection created was
#  COLL_NAME = /tempZone/home/rods/ruletest/sub1
  
  msiCollCreate(*Path,"0",*Status);

  #  Verify collection was created
  writeLine("stdout","Create collection *Path");
  writeLine("stdout","Collection created was");
  msiExecStrCondQuery("SELECT COLL_NAME where COLL_NAME = '*Path'", *QOut);
  foreach(*QOut) { msiPrintKeyValPair("stdout",*QOut); }
}
INPUT *Path="/tempZone/home/rods/ruletest/sub1"
OUTPUT ruleExecOut
