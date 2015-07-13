myTestRule {
#Input parameter is:
#  Original property list
#Output parameter is:
#  Cloned property list

  #Create key-value string
  msiString2KeyValPair(*Str,*KVpair);

  #Write out string
  writeLine("stdout","Initial property list is");
  msiPrintKeyValPair("stdout",*KVpair);

  #Clone the string
  msiPropertiesClone(*KVpair,*KVpair2);

  #Write out cloned string
  writeLine("stdout","Cloned property list is");
  msiPrintKeyValPair("stdout",*KVpair2);
}
INPUT *Str="key1=value1", *Keyword="key2", *Value="value2"
OUTPUT ruleExecOut
