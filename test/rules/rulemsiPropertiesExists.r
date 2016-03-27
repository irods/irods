myTestRule {
#Input parameters are:
#  Property list
#  Keyword to find
#Output parameter is:
#  Boolean result (1 if keyword is present)

  #Create key-value string
  msiString2KeyValPair(*Str,*KVpair);

  #Write out string
  writeLine("stdout","Initial property list is");
  msiPrintKeyValPair("stdout",*KVpair);

  #Verify property exists
  msiPropertiesExists(*KVpair,*Keyword,*Bvalue);
  writeLine("stdout","Property list checked for existence of *Keyword");
  writeString("stdout","Result is ");
  if(*Bvalue) {
    writeLine("stdout","Keyword *Keyword exists");
  }
  else {
    writeLine("stdout","Keyword *Keyword does not exist");
  }
}
INPUT *Str="key1=value1", *Keyword="key2", *Value="value2"
OUTPUT ruleExecOut
