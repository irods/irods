myTestRule {
#Input parameters are:
#  Property list
#  Keyword to find
#Output parameter is:
#  Value

  #Create key-value string
  msiString2KeyValPair(*Str,*KVpair);

  #Output property list
  writeLine("stdout","Property list is");
  msiPrintKeyValPair("stdout",*KVpair);

  #Extract property value
  msiPropertiesGet(*KVpair,*Str1,*Val);
  writeLine("stdout","Properties list keyword *Str1 has value *Val");
}
INPUT *Str="key1=value1", *Str1="key1"
OUTPUT ruleExecOut
