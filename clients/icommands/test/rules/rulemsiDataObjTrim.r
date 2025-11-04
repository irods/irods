myTestRule {
#Input parameters are:
#  Data object path
#  Optional storage resource name
#  Optional replica number
#  Optional number of replicas to keep
#  Optional administrator flag irodsAdmin, to enable administrator to trim replicas
#Output parameter is:
#  Status
#Output from running the example is:
#  The replicas of File /tempZone/home/rods/sub1/foo2 are deleted
  msiDataObjTrim(*SourceFile,"null","null","1","null",*Status);
  writeLine("stdout","The replicas of file *SourceFile are deleted");
}
INPUT *SourceFile="/tempZone/home/rods/sub1/foo2"
OUTPUT ruleExecOut
