myTestRule {
#Input parameters are:
#  String of tagged values
#Output parameter is:
#   Property list
#Parse string into a property list
  msiPropertiesFromString(*Str,*KVpair);
  writeLine("stdout","Property list is");
  msiPrintKeyValPair("stdout",*KVpair);
}
INPUT *Str="<key1>value1</key1>,<key2>value2</key2>"
OUTPUT ruleExecOut
