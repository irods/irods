myTestRule {
#This microservice is deprecated
#Input parameters are:
#  Data object path
#  Optional storage resource
#  Optional parameter, valid flags are:
#    all    - to specify replicate to all resources in storage group
#    irodsAdmin - to enable administrator to replicate other users' files
  msiDataObjReplWithOptions(*SourceFile,*Resource,"null",*Status);
  writeLine("stdout","The file *SourceFile is replicated onto resource *Resource");
}
INPUT *SourceFile="/tempZone/home/rods/sub1/foo2", *Resource="testResc" 
OUTPUT ruleExecOut
