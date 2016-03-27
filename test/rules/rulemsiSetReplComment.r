myTestRule {
#Input parameters are:
#  Object ID if known
#  Data object path
#  Replica number
#  Comment to be added
#Output parameter is:
#  Status
#Output from running the example is:
#  The comment added to file /tempZone/home/rods/sub1/foo3 is "New comment" 
#  The comment retrieved from iCAT is "New comment"
   msiSetReplComment("null",*SourceFile,0,*Comment);
   writeLine("stdout","The comment added to file *SourceFile is *Comment");
   msiSplitPath(*SourceFile,*Coll,*File);
   msiMakeGenQuery("DATA_COMMENTS","DATA_NAME = '*File' AND COLL_NAME = '*Coll'",*GenQInp);
   msiExecGenQuery(*GenQInp,*GenQOut);
   foreach(*GenQOut) {
     msiGetValByKey(*GenQOut,"DATA_COMMENTS",*com);
     writeLine("stdout","The comment retrieved from iCAT is *com");
   }
}
INPUT *SourceFile="/tempZone/home/rods/sub1/foo3", *Comment="New comment"
OUTPUT ruleExecOut
