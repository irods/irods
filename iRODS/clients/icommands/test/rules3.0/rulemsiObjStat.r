myTestRule {
#Input parameter is:
#  Data object path
#Output parameter is:
#  Type of object is written into a RodsObjStat_PI structure
   msiSplitPath(*SourceFile,*Coll,*File);
   msiObjStat(*SourceFile,*Stat);
   msiObjStat(*Coll,*Stat1);
   writeLine("stdout","Type of object is written into a RodsObjStat_PI structure");
}
INPUT *SourceFile="/tempZone/home/rods/sub1/foo3"
OUTPUT ruleExecOut
