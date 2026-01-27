myTestRule {
#Input parameters are:
#  String with conditional query
#Output parameter is:
#  Result string
  msiExecStrCondQuery(*Select,*QOut);
  foreach(*QOut) {
    msiPrintKeyValPair("stdout",*QOut)
  }
}
INPUT *Select="SELECT DATA_NAME where DATA_NAME like 'rule%%'"
OUTPUT ruleExecOut 
