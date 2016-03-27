myTestRule {
#Output parameter is:
#  Property list to create

  #Create new property list
  msiPropertiesNew(*KVpair);

  #Add property to list
  msiPropertiesAdd(*KVpair,*Keyword,*Value);

  #Write out property list
  writeLine("stdout","Property list is");
  msiPrintKeyValPair("stdout",*KVpair);
}
INPUT *Keyword="key2", *Value="value2"
OUTPUT ruleExecOut
