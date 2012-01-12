myTestRule {
#Input parameter is:
#  Input string - %-separated key=value strings
#Output parameter is:
#  String array buffer
  writeLine("stdout","Input string is *Str");
  msiString2KeyValPair(*Str,*Keyval);
  writeKeyValPairs("stdout",*Keyval," : ");
  msiString2StrArray(*Str,*Stray);
  msiStrArray2String(*Stray, *Str2);
  writeLine("stdout","After conversion to array and back, string is");
  writeLine("stdout",*Str2);
}
INPUT *Str="key1=value1%key2=value2%key3=value3"
OUTPUT ruleExecOut
