integrityFileOwner {
#Input parameter is:
#  Name of collection that will be checked
#  Owner name that will be verified
#Output is:
#  List of all files in the collection that have a different owner

  #Verify that input path is a collection
  msiIsColl(*Coll,*Result, *Status);
  if(*Result == 0) {
    writeLine("stdout","Input path *Coll is not a collection");
    fail;
  }
  *ContInxOld = 1;
  *Count = 0;

  #Loop over files in the collection
  msiMakeGenQuery("DATA_ID,DATA_NAME,DATA_OWNER_NAME","COLL_NAME = '*Coll'",*GenQInp);
  msiExecGenQuery(*GenQInp, *GenQOut);
  msiGetContInxFromGenQueryOut(*GenQOut,*ContInxNew);
  while(*ContInxOld > 0) {
    foreach(*GenQOut) {
      msiGetValByKey(*GenQOut,"DATA_OWNER_NAME",*Attrname);
      if(*Attrname != *Attr) { 
        msiGetValByKey(*GenQOut,"DATA_NAME",*File);
        writeLine("stdout","File *File has owner *Attrname");
        *Count = *Count + 1;
      }
    }
    *ContInxOld = *ContInxNew;
    if(*ContInxOld > 0) {msiGetMoreRows(*GenQInp,*GenQOut,*ContInxNew);}
  }
  writeLine("stdout","Number of files in *Coll with owner other than *Attr is *Count");
}
INPUT *Coll = "/tempZone/home/rods/sub1", *Attr = "rods"
OUTPUT ruleExecOut
