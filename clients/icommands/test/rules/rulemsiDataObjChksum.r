myTestRule {
# Input parameters are:
#  Data object path
#  Optional flags in form Keyword=value
#    ChksumAll=
#    verifyChksum=
#    forceChksum=
#    replNum=
# Output parameters are:
#  Checksum value
# Output from running the example is
#  Collection is /tempZone/home/rods/sub1 and file is foo1
#  Saved checksum for file foo1 is f03e80c9994d137614935e4913e53417, new checksum is f03e80c9994d137614935e4913e53417 
   msiSplitPath(*dataObject,*Coll,*File);
   writeLine("stdout","Collection is *Coll and file is *File");
   msiMakeGenQuery("DATA_CHECKSUM","DATA_NAME = '*File' AND COLL_NAME = '*Coll'",*GenQInp);
   msiExecGenQuery(*GenQInp,*GenQOut);
   foreach(*GenQOut) {
     msiGetValByKey(*GenQOut,"DATA_CHECKSUM",*chkSumS);
     msiDataObjChksum(*dataObject,*Flags,*chkSum);
     writeLine("stdout","Saved checksum for file *File is *chkSumS, new checksum is *chkSum");
  }
}
INPUT *dataObject="/tempZone/home/rods/sub1/foo1", *Flags="forceChksum="
OUTPUT ruleExecOut
