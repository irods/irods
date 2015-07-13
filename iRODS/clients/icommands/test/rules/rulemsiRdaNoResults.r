myTestRule {
#Input parameters are:
#  Name of RDA database access file
#  SQL string to be sent to RDA database
#  3-6 optional bind variables for SQL string
  msiRdaNoResults(*rda, *sql, "null", "null", "null", "null");   
  msiRdaCommit;
}
INPUT *rda="RDA", *sql="create table t2 (c1 varchar(20), c2 varchar(20))"
OUTPUT ruleExecOut
