myTestRule {
#Input parameters are:
#  Key-value buffer (may be empty)
#  Key
#  Value
  msiGetSystemTime(*Time,"human");
  msiAddKeyVal(*Keyval,*Key,*Time);
  msiAssociateKeyValuePairsToObj(*Keyval,*Coll,"-C");
  msiGetCollectionPSmeta(*Coll,*Buf);
  writeBytesBuf("stdout",*Buf);
}
INPUT *Coll="/tempZone/home/rods/sub1", *Key="TimeStamp"
OUTPUT ruleExecOut
