integrityAVUvalue {
#Verify each file in a collection has a required attribute name and attribute value
#Input parameter is:
#  Name of collection that will be checked
#  Attribute name whose presence will be verified
#  Attribute value that will be verified
#Output is:
#  List of all files in the collection that are either missing the attribute
#  of have the wrong attribute value

  #Verify that input path is a collection
  msiIsColl(*Coll,*Result, *Status);
  if(*Result == 0) {
    writeLine("stdout","Input path *Coll is not a collection");
    fail;  }
  *ContInxOld = 1;
  *Count = 0;

  #Loop over files in the collection
  msiMakeGenQuery("DATA_ID,DATA_NAME","COLL_NAME = '*Coll'", *GenQInp);
  msiExecGenQuery(*GenQInp, *GenQOut);
  msiGetContInxFromGenQueryOut(*GenQOut,*ContInxNew);
  while(*ContInxOld > 0) {
    foreach(*GenQOut) {
      msiGetValByKey(*GenQOut,"DATA_ID", *Dataid);
      msiGetValByKey(*GenQOut,"DATA_NAME", *File);
      msiMakeGenQuery("META_DATA_ATTR_NAME","DATA_ID = '*Dataid'", *GenQInp1);
      msiExecGenQuery(*GenQInp1, *GenQOut1);
      *Attrfound = 0;
      foreach(*GenQOut1) {
        msiGetValByKey(*GenQOut1,"META_DATA_ATTR_NAME",*Attrname);
        if(*Attrname == *Attr) { 
          *Attrfound = 1;
          msiMakeGenQuery("META_DATA_ATTR_VALUE","DATA_ID = '*Dataid' and META_DATA_ATTR_NAME = '*Attrname'",*GenQinp2);
          msiExecGenQuery(*GenQinp2,*GenQOut2);
          foreach(*GenQOut2) {
            msiGetValByKey(*GenQOut2,"META_DATA_ATTR_VALUE",*Attrval);
            if(*Attrval != "*Attrv") {writeLine("stdout","Incorrect attribute value for *Attr in *File in *Coll");}
          } 
        }
      }
      if(*Attrfound == 0) {
         writeLine("stdout","*File does not have attribute *Attr");
         *Count = *Count + 1;
      }
    }
    *ContInxOld = *ContInxNew;
    if(*ContInxOld > 0) {msiGetMoreRows(*GenQInp,*GenQOut,*ContInxNew);}
  }
  writeLine("stdout","Number of files in *Coll missing attribute *Attr is *Count");
}
INPUT *Coll = "/tempZone/home/rods/sub1", *Attr = "Event", *Attrv = "document"
OUTPUT ruleExecOut
