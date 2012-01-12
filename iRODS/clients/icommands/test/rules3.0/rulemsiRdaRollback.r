myTestRule {
#Input parameter:
#  None
  msiRdaNoResults(*rda, *sql, "null", "null", "null", "null") ::: msiRdaRollback;   
  msiRdaCommit;
}
INPUT *rda="RDA", *sql="create table t2 (c1 varchar(20), c2 varchar(20))"
OUTPUT ruleExecOut
