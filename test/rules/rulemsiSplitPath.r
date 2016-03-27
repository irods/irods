myTestRule {
#Input parameter is:
#   Data object path
#Output parameters are:
#  Collection name
#  File name
#Output from running the example is:
#  Object is /tempZone/home/rods/sub1/foo1
#  Collection is /tempZone/home/rods/sub1 and file is foo1
   writeLine("stdout","Object is *dataObject");
   msiSplitPath(*dataObject,*Coll,*File);
   writeLine("stdout","Collection is *Coll and file is *File");
}
INPUT *dataObject="/tempZone/home/rods/sub1/foo1"
OUTPUT ruleExecOut
