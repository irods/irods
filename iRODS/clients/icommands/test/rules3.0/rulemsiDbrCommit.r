myTestRule {
# Input parameters are:
#  Database resource name
# Output from running the example is:
#  Changes to database resource dbr2 were committed
  msiDbrCommit(*DBR);
  writeLine("stdout","Changes to database resource *DBR were committed");
}
INPUT *DBR="dbr2"
OUTPUT ruleExecOut
