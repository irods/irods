myTestRule {
#Input parameters are:
#  Attribute list
#  Condition for selecting files
#Output parameter is:
#  SQL execution string
#Output from running the example is:
#  List of all files that start with rule
  msiMakeQuery(*Select,*Condition,*Query);
  msiExecStrCondQuery(*Query, *GenQOut);
  foreach(*GenQOut) {msiPrintKeyValPair("stdout",*GenQOut);}
}
INPUT *Select="DATA_NAME, COLL_NAME, DATA_RESC_NAME, DATA_REPL_NUM, DATA_SIZE", *Condition="DATA_NAME like 'rule%%'"
OUTPUT ruleExecOut
