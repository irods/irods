myTestRule {
#Input parameter is:
#  Path of collection
#Output parameter is:
#  Buffer holding the result

  #Add metadata to a collection
  msiString2KeyValPair(*Keyvalstr, *Keyval);
  msiAssociateKeyValuePairsToObj(*Keyval,*Path,"-C");
  
  #Retrieve metadata for the collection
  msiGetCollectionPSmeta(*Path,*Buf);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Path="/tempZone/home/rods/sub1", *Keyvalstr="Testdate=6/10/2011"
OUTPUT ruleExecOut
