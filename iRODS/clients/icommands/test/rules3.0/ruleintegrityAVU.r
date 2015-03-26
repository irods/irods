integrityAVU {
#Input parameter is:
#  Name of collection that will be checked
#  Attribute name whose presence will be verified
#Output is:
#  List of all files in the collection that are missing the attribute

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
      msiMakeGenQuery("META_DATA_ATTR_NAME","DATA_ID = '*Dataid'", *GenQInp1);
      msiExecGenQuery(*GenQInp1, *GenQOut1);
      *Attrfound = 0;
      foreach(*GenQOut1) {
        msiGetValByKey(*GenQOut1,"META_DATA_ATTR_NAME",*Attrname);
        if(*Attrname == *Attr) { 
           *Attrfound = 1;
        }
      }
      msiFreeBuffer(*GenQInp1);
      msiFreeBuffer(*GenQOut1);
      if(*Attrfound == 0) {
        msiGetValByKey(*GenQOut,"DATA_NAME", *File);
        writeLine("stdout","*File does not have attribute *Attr");
        *Count = *Count + 1;
      }
    }
    *ContInxOld = *ContInxNew;
    if(*ContInxOld > 0) {msiGetMoreRows(*GenQInp,*GenQOut,*ContInxNew);}
  }
  writeLine("stdout","Number of files in *Coll missing attribute *Attr is *Count");
}
INPUT *Coll = "/tempZone/home/rods/sub1", *Attr = "DC.Relation"
OUTPUT ruleExecOut
