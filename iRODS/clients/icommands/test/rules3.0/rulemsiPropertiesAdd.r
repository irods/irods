myTestRule {
#Input parameters are:
#  Property list
#  Keyword to add
#  Value to add

  #Create key-value string
  msiString2KeyValPair(*Str,*KVpair);

  #Write out string
  writeLine("stdout","Initial property list is");
  msiPrintKeyValPair("stdout",*KVpair);

  #Add to properties list
  msiPropertiesAdd(*KVpair,*Keyword,*Value);

  #Write out properties list
  writeLine("stdout","Changed property list is");
  msiPrintKeyValPair("stdout",*KVpair);
}
INPUT *Str="key1=value1", *Keyword="key2", *Value="value2"
OUTPUT ruleExecOut
