myTestRule {
#Input parameters are:
#  String with %-separated key=value pair strings
#Output parameter is:
#  Key-value structure
  writeLine("stdout","Add metadata string *Str to *Path");
  msiString2KeyValPair(*Str,*Keyval);
  writeKeyValPairs("stdout",*Keyval,*Status);
  msiAssociateKeyValuePairsToObj(*Keyval,*Path,"-d");
  msiGetDataObjPSmeta(*Path,*Buf);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Str="Testing=yes%Event=moon landing in space!", *Path="/tempZone/home/rods/test/ERAtestfile.txt"
OUTPUT ruleExecOut
