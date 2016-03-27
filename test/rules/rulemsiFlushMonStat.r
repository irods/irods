myTestRule {
#Input parameters are:
#  Timespan before which stats are deleted (in hours)
#  Table to be flushed
#    serverload
#    serverloaddigest
#Output from running the example is a list of load factors per resource
  msiFlushMonStat(*Time, *Table);
  msiDigestMonStat(*Cpuw, *Memw, *Swapw, *Runw, *Diskw, *Netinw, *Netow);
  msiExecStrCondQuery("SELECT SLD_RESC_NAME,SLD_LOAD_FACTOR",*QOut);
  foreach(*QOut) {msiPrintKeyValPair("stdout",*QOut); }
}
INPUT *Time="24", *Table="serverload", *Cpuw="1", *Memw="1", *Swapw="0", *Runw="0", *Diskw="0", *Netinw="1", *Netow="1"
OUTPUT ruleExecOut
