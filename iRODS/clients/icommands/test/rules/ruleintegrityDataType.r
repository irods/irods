integrityDataType {
#Input parameter is:
#  Name of collection that will be checked
#  Data type that will be verified
#Output is:
#  List of all files in the collection that have a different data type

  #Verify that input path is a collection
  msiIsColl(*Coll,*Result, *Status);
  if(*Result == 0) {
    writeLine("stdout","Input path *Coll is not a collection");
    fail;
  }

  #Get object type number from input type
  *ContInxOld = 1;
  *Count = 0;

  #Loop over files in the collection
  msiMakeGenQuery("DATA_NAME,DATA_TYPE_NAME","COLL_NAME = '*Coll'", *GenQInp);
  msiExecGenQuery(*GenQInp, *GenQOut);
  msiGetContInxFromGenQueryOut(*GenQOut,*ContInxNew);
  while(*ContInxOld > 0) {
    foreach(*GenQOut) {
      msiGetValByKey(*GenQOut,"DATA_TYPE_NAME",*Attrname);
      msiGetValByKey(*GenQOut,"DATA_NAME",*File);
      if(*Attrname != *Attr) { 
        writeLine("stdout","*File has data type *Attrname");
        *Count = *Count + 1;
      }
    }
    *ContInxOld = *ContInxNew;
    if(*ContInxOld > 0) {msiGetMoreRows(*GenQInp,*GenQOut,*ContInxNew);}
  }
  writeLine("stdout","Number of files in *Coll without data type *Attr is *Count");
}
INPUT *Coll = "/tempZone/home/rods/sub1", *Attr = "text"
OUTPUT ruleExecOut
