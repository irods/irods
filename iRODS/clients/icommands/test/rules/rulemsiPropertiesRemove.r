myTestRule {
#Input parameters are:
#  Properties list
#  Keyword to remove from list

  #Create properties list
  msiString2KeyValPair(*Str,*KVpair);

  #Add a property
  msiPropertiesAdd(*KVpair,*Keyword,*Value);

  #Write out property list
  writeLine("stdout","Property list is");
  msiPrintKeyValPair("stdout",*KVpair);

  #Remove a property
  msiPropertiesRemove(*KVpair,*Keyword);

  #Write out revised property list
  writeLine("stdout","Changed property list is");
  msiPrintKeyValPair("stdout",*KVpair);
}
INPUT *Str="key1=value1", *Keyword="key2", *Value="value2"
OUTPUT ruleExecOut
