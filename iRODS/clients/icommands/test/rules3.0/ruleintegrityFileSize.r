integrityFileSize {
#Input parameter is:
#  Name of collection that will be checked
#  Minimum file size allowed in collection
#  Maximum file size allowed in collection
#Output is:
#  List of all files in the collection that have a size outside the allowed range

  #Verify that input path is a collection
  *Isizemax = int(*Sizemax);
  *Isizemin = int(*Sizemin);
  msiIsColl(*Coll,*Result, *Status);
  if(*Result == 0) {
    writeLine("stdout","Input path *Coll is not a collection");
    fail;
  }
  *ContInxOld = 1;
  *Count = 0;

  #Loop over files in the collection
  msiMakeGenQuery("DATA_ID,DATA_NAME,DATA_SIZE","COLL_NAME = '*Coll'", *GenQInp);
  msiExecGenQuery(*GenQInp, *GenQOut);
  msiGetContInxFromGenQueryOut(*GenQOut,*ContInxNew);
  while(*ContInxOld > 0) {
    foreach(*GenQOut) {
      msiGetValByKey(*GenQOut,"DATA_SIZE",*Dsize);
      *Idsize = int(*Dsize);
      if(*Idsize > *Isizemax) { 
        msiGetValByKey(*GenQOut,"DATA_NAME",*File);
        writeLine("stdout","File *File with size *Dsize is larger than allowed size");
        *Count = *Count + 1;
      }
      if(*Idsize < *Isizemin) {
        msiGetValByKey(*GenQOut,"DATA_NAME",*File);
        writeLine("stdout","File *File with size *Dsize is smaller than allowed size");
        *Count = *Count + 1;
      }
    }
    *ContInxOld = *ContInxNew;
    if(*ContInxOld > 0) {msiGetMoreRows(*GenQInp,*GenQOut,*ContInxNew);}
  }
  writeLine("stdout","Number of files in *Coll outside size range *Sizemin to *Sizemax is *Count");
}
INPUT *Coll = "/tempZone/home/rods/sub1", *Sizemin = "1000000", *Sizemax = "1000000000"
OUTPUT ruleExecOut
