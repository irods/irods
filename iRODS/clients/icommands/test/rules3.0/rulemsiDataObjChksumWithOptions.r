myTestRule {
# This example is deprecated
# Input parameters are
#  Data object path
#  Optional flag for verifyChksum or forceChksum
#  Optional flag for ChksumAll or an integer for the replica number
# Output parameter is:
#  Checksum value
# Output from running the example is:
#  Collection is /tempZone/home/rods/sub1 and file is foo1
#  Saved checksum for file foo1 is f03e80c9994d137614935e4913e53417, new checksum is f03e80c9994d137614935e4913e53417
  msiSplitPath(*Path,*Coll,*File);
  writeLine("stdout","Collection is *Coll and file is *File");
  msiMakeGenQuery("DATA_CHECKSUM","DATA_NAME = '*File' AND COLL_NAME = '*Coll'",*GenQInp);
  msiExecGenQuery(*GenQInp,*GenQOut);
  foreach(*GenQOut) {
    msiGetValByKey(*GenQOut,"DATA_CHECKSUM",*chkSumS);
    msiDataObjChksumWithOptions(*Path,*Flags,0,*chkSum);
    writeLine("stdout","Saved checksum for file *File is *chkSumS, new checksum is *chkSum");
  }
}
INPUT *Path="/tempZone/home/rods/sub1/foo1", *Flags="forceChksum"
OUTPUT ruleExecOut
