myTestRule {
#Input parameter is:
#  Object name
#Output parameter is:
#  Type
#Output from running the example is:
#  The type of object /tempZone/home/rods/sub1/foo3 is -d
#  The type of object demoResc is -r
  msiGetObjType(*SourceFile,*Type);
  writeLine("stdout","The type of object *SourceFile is *Type");
  msiGetObjType(*Resource,*Type1);
  writeLine("stdout","The type of object *Resource is *Type1");
}
INPUT *SourceFile="/tempZone/home/rods/sub1/foo3", *Resource="demoResc"
OUTPUT ruleExecOut
