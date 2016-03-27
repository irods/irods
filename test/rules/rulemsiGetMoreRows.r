myTestRule {
#Input parameters are:
#  Structure holding the query
#  Structure holding the query result
#Output parameter is:
#  Continuation index, greater than zero is additional rows can be retrieved
#Output from running the example is:
#  List of the number of files and size of files in collection /tempZone/home/rods
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
INPUT *Coll = "/tempZone/home/rods/%%", *Condition="COLL_NAME like '*Coll'"
OUTPUT ruleExecOut
