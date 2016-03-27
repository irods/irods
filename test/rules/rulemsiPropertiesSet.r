myTestRule {
#Input parameters are:
#  Properties list
#  Keyword to check
#  Value to set

  #Create property string
  msiString2KeyValPair(*Str,*KVpair);
  msiPropertiesAdd(*KVpair,*Keyword,*Value);

  #Write property list
  writeLine("stdout","Property list is");
  msiPrintKeyValPair("stdout",*KVpair);

  #Change the value of a property in the list
  msiPropertiesSet(*KVpair,*Keyword,*Val2);

  #Write out new property list
  writeLine("stdout","Changed property list is");
  msiPrintKeyValPair("stdout",*KVpair);
}
INPUT *Str="key1=value1", *Keyword="key2", *Value="value2", *Val2="newvalue2"
OUTPUT ruleExecOut
