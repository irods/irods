myTestRule {
#Input parameters are:
#  Structure holding the query
#Output parameter is:
#  Structure holding the query result
#Output from running the example is:
#  List of the number of files and size of files in collection /tempZone/home/rods/large-coll
  *ContInxOld = 1;
  *Count = 0;
  *Size = 0;
  msiMakeGenQuery("DATA_ID, DATA_SIZE",*Condition,*GenQInp);
  msiExecGenQuery(*GenQInp, *GenQOut);
  msiGetContInxFromGenQueryOut(*GenQOut,*ContInxNew);
  while(*ContInxOld > 0) {
    foreach(*GenQOut) {
       msiGetValByKey(*GenQOut, "DATA_SIZE", *Fsize);
       *Size = *Size + double(*Fsize);
       *Count = *Count + 1;
    }
    *ContInxOld = *ContInxNew;
    if(*ContInxOld > 0) {msiGetMoreRows(*GenQInp,*GenQOut,*ContInxNew);}
  }
  writeLine("stdout","Number of files in *Coll is *Count and total size is *Size");
}
INPUT *Coll = "/tempZone/home/rods/large-coll", *Condition="COLL_NAME like '*Coll'"
OUTPUT ruleExecOut

