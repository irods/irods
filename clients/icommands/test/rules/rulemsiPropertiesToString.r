myTestRule {
#Input parameters are:
#  Property list
#Output parameter is:
#  String

  #Create property string
  msiString2KeyValPair(*Str,*KVpair);

  #Write out property list
  writeLine("stdout","Initial property list is");
  msiPrintKeyValPair("stdout",*KVpair);

  #Convert to a string
  msiPropertiesToString(*KVpair,*Strout);

  #Write out string
  writeLine("stdout","Generated string");
  writeLine("stdout","*Strout");
}
INPUT *Str="key1=value1%key2=value2"
OUTPUT ruleExecOut
