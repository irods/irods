myTestRule {
#Input parameters are:
#  Key-value pair list
#  Path to object
#  Type of object (-d, -C)
#Output from running the example is:
#  Add metadata

  msiString2KeyValPair(*Str,*Keyval);
  msiAssociateKeyValuePairsToObj(*Keyval,*Path,"-d");

  #  List metadata
  writeLine("stdout","List metadata on file");
  msiGetDataObjPSmeta(*Path,*Buf);
  writeBytesBuf("stdout",*Buf);

  #  Remove metadata
  msiRemoveKeyValuePairsFromObj(*Keyval,*Path,"-d");

  #  List metadata remaining on file
  writeLine("stdout","list metadata after removing *Str");
  msiGetDataObjPSmeta(*Path,*Buf);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Path="/tempZone/home/rods/sub1/foo3", *Str="Testmeta=deletetest"
OUTPUT ruleExecOut
