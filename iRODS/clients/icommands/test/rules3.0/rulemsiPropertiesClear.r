myTestRule {
#Input parameter is:
#  Property list

  #Create key-value string
  msiString2KeyValPair(*Str,*KVpair);

  #Write out strin
  writeLine("stdout","Initial property list is");
  msiPrintKeyValPair("stdout",*KVpair);

  #Clear properties list
  msiPropertiesClear(*KVpair);
  
  #Verify property was cleared
  writeLine("stdout","Changed property list is null");
  msiPrintKeyValPair("stdout",*KVpair);
}
INPUT *Str="key1=value1"
OUTPUT ruleExecOut
