myTestRule {
#Input parameters are:
#  CPU weight
#  Memory weight
#  Swap weight
#  Run queue weight
#  Disk weight
#  Network transfer in weight
#  Network transfer out weight
#Output from running the example is:
#  CPU weight is 1, Memory weight is 1, Swap weight is 0, Run queue weight is 0
#  Disk weight is 0, Network transfer in rate is 1, Network transfer out rate is 1
#  List of resources and the computed load factor digest
  msiDigestMonStat(*Cpuw, *Memw, *Swapw, *Runw, *Diskw, *Netinw, *Netow);
  writeLine("stdout","CPU weight is *Cpuw, Memory weight is *Memw, Swap weight is *Swapw, Run queue weight is *Runw");
  writeLine("stdout","Disk weight is *Diskw, Network transfer in rate is *Netinw, Network transfer out rate is *Netow");
  msiExecStrCondQuery("SELECT SLD_RESC_NAME,SLD_LOAD_FACTOR",*QOut);
  foreach(*QOut) { msiPrintKeyValPair("stdout",*QOut) }
}
INPUT *Cpuw="1", *Memw="1", *Swapw="0", *Runw="0", *Diskw="0", *Netinw="1", *Netow="1"
OUTPUT ruleExecOut
