myTestRule {
#Input parameter is:
#  Data object structure
#Output parameters are:
#  Data name converted to a string
#  Status
  *Work=``{
             msiDataObjChksum(*File,"ChksumAll",*chk);
             # convert from object structure to a string for printing
             msiGetObjectPath(*File,*obj,*status);
             writeLine("stdout",'Checksum for *obj is *chk');
          }``;
  msiCollectionSpider(*Coll,*File,*Work,*Status);
  writeLine("stdout","Operations completed");
}
INPUT *Coll="/tempZone/home/rods/sub1"
OUTPUT ruleExecOut
