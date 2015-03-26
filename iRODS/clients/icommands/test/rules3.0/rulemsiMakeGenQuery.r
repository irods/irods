myTestRule {
#Input parameters are:
#  Selected attribute list
#  Condition for selecting files
#Output parameter is:
#  Structure holding the query
#Output from running the example is:
#  List of the number of files and size of files in collection /tempZone/home/rods
  *ContInxOld = 1;
  *Count = 0;
  *Size = 0;
  msiMakeGenQuery("DATA_ID, DATA_SIZE",*Condition,*GenQInp);
  msiExecGenQuery(*GenQInp, *GenQOut);
  msiGetContInxFromGenQueryOut(*GenQOut,*ContInxNew);
  while(*ContInxOld > 0) {
    if(*ContInxNew == 0) { *ContInxOld = 0; }
    foreach(*GenQOut) {
       msiGetValByKey(*GenQOut, "DATA_SIZE", *Fsize);
       *Size = *Size + double(*Fsize);
       *Count = *Count + 1;
    }
    if(*ContInxOld > 0) {msiGetMoreRows(*GenQInp,*GenQOut,*ContInxNew);}
  }
  writeLine("stdout","Number of files in *Coll is *Count and total size is *Size");
}
INPUT *Coll = "/tempZone/home/rods/%%", *Condition="COLL_NAME like '*Coll'"
OUTPUT ruleExecOut
