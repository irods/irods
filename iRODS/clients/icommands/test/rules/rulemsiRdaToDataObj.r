myTestRule {
#Input parameters are:
#  Name of RDA database access file
#  SQL string that will be sent to the database
#  1-4 Optional bind variables for the SQL statement
#  File descriptor into which the results will be written  
  msiDataObjCreate(*Path, *Oflag, *D_FD);
  msiRdaToDataObj(*rda,*sql,"null","null","null","null",*D_FD); 
  msiDataObjClose(*D_FD,*Status);
}
INPUT *rda="RDA", *sql="select * from t2", *Path="/tempZone/home/rods/test/sqlOut.1", *Oflag="forceFlag="
OUTPUT ruleExecOut
