integrityExpiry {
#Input parameter is:
#  Name of collection that will be checked
#  Flag for "EXPIRED" or for "NOT EXPIRED"
#Output is:
#  List of all files in the collection that have either EXPIRED or NOT EXPIRED

  #Verify that input path is a collection
  msiIsColl(*Coll,*Result, *Status);
  if(*Result == 0) {
    writeLine("stdout","Input path *Coll is not a collection");
    fail;
  }
  *ContInxOld = 1;
  *Count = 0;
  *Counte = 0;
  msiGetIcatTime(*Time,"unix");

  #Loop over files in the collection
  msiMakeGenQuery("DATA_ID,DATA_NAME,DATA_EXPIRY","COLL_NAME = '*Coll'", *GenQInp);
  msiExecGenQuery(*GenQInp, *GenQOut);
  msiGetContInxFromGenQueryOut(*GenQOut,*ContInxNew);
  while(*ContInxOld > 0) {
    foreach(*GenQOut) {
      msiGetValByKey(*GenQOut,"DATA_EXPIRY",*Attrname);
      if(*Attrname > *Time  && *Flag == "NOT EXPIRED") { 
        msiGetValByKey(*GenQOut,"DATA_NAME",*File);
        writeLine("stdout","File *File has not expired");
        *Count = *Count + 1;
      }
      if(*Attrname <= *Time && *Flag == "EXPIRED") {
        msiGetValByKey(*GenQOut,"DATA_NAME",*File);
        writeLine("stdout","File *File has expired");
        *Counte = *Counte + 1;
      }
      *ContInxOld = *ContInxNew;
      if(*ContInxOld > 0) {msiGetMoreRows(*GenQInp,*GenQOut,*ContInxNew);}
    }
  }
  if(*Flag == "EXPIRED") {writeLine("stdout","Number of files in *Coll that have expired is *Counte");}
  if(*Flag == "NOT EXPIRED") {writeLine("stdout","Number of files in *Coll that have not expired is *Count");}
}
INPUT *Coll = "/tempZone/home/rods/sub1", *Flag = "EXPIRED"
OUTPUT ruleExecOut
