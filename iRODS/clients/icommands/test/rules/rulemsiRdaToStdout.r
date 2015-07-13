myTestRule {
#Input parameters are:
#  Name of RDA database access file
#  SQL string to be sent to RDA database
#  1-4 Optional bind variables for SQL string
  msiRdaToStdout(*rda,*sql,"null","null","null","null");
}
INPUT *rda="RDA", *sql="select * from t2"
OUTPUT ruleExecOut
