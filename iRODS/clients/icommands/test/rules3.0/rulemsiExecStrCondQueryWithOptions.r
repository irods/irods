myTestRule {
#Input parameters are:
# String containing conditional query
# Optional flag to allow null return
# Optional maximum number of rows returned
#   zeroOK
#Output parameter is:
# Results from executing the query
  msiMakeQuery(*Sel,*Cond,*QIn);
  msiExecStrCondQueryWithOptions(*QIn,"zeroOK","15",*QOut);
  foreach(*QOut) {msiPrintKeyValPair("stdout",*QOut);}

  # see if the result is the same from an alternate microservice
  writeLine("stdout","Compare output with msiExecStrCondQuery results");
  msiExecStrCondQuery(*QIn,*QOut);
  *Count = 0;
  foreach(*QOut) {
    *Count = *Count + 1;
    if(*Count <= 15) {msiPrintKeyValPair("stdout",*QOut);}
  }
}
INPUT *Sel="DATA_NAME", *Cond="DATA_NAME like 'rule%%'"
OUTPUT ruleExecOut
